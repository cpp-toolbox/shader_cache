#include "shader_cache.hpp"
#include "sbpt_generated_includes.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fstream>
#include <sstream>

/**
 *
 * \pre there is an active opengl context, otherwise undefined behavior
 * \param requested_shaders out of the passed in which ones to actually create on instantiation
 */
ShaderCache::ShaderCache(std::vector<ShaderType> requested_shaders) {
    // Initialize shader programs based on the predefined mapping
    for (const auto &shader_type : requested_shaders) {
        create_shader_program(shader_type);
    }
    this->log_shader_program_info();
}

ShaderCache::ShaderCache(std::vector<ShaderType> requested_shaders, const std::vector<spdlog::sink_ptr> &sinks)
    : ShaderCache(requested_shaders) {
    logger_component = LoggerComponent("shader cache", sinks);
}

ShaderCache::~ShaderCache() {
    for (auto &pair : created_shaders) {
        glDeleteProgram(pair.second.id);
    }
}

/**
 * \pre the requested shader program has been created
 * \param type the type of shader to get
 * \return the id of the shader program
 */
ShaderProgramInfo ShaderCache::get_shader_program(ShaderType type) const {
    auto it = created_shaders.find(type);
    if (it != created_shaders.end()) {
        return it->second;
    }
    throw std::runtime_error("Shader program not found");
}

void ShaderCache::use_shader_program(ShaderType type) {
    ShaderProgramInfo shader_info = get_shader_program(type);
    glUseProgram(shader_info.id);
}

void ShaderCache::stop_using_shader_program() { glUseProgram(0); }

void ShaderCache::create_shader_program(ShaderType type) {

    bool logging = logger_component.logging_enabled;
    auto logger = logger_component.get_logger();

    auto it = shader_standard.shader_catalog.find(type);
    if (it == shader_standard.shader_catalog.end()) {
        throw std::runtime_error("Shader type not found");
    }

    if (logging) {
        logger->info("creating new shader program");
    }

    const ShaderCreationInfo &shader_info = it->second;

    GLuint shader_program = glCreateProgram();
    GLuint vertex_shader = attach_shader(shader_program, shader_info.vertex_path, GL_VERTEX_SHADER);
    GLuint fragment_shader = attach_shader(shader_program, shader_info.fragment_path, GL_FRAGMENT_SHADER);
    GLuint geometry_shader = 0;

    if (!shader_info.geometry_path.empty()) {
        geometry_shader = attach_shader(shader_program, shader_info.geometry_path, GL_GEOMETRY_SHADER);
    }

    link_program(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (geometry_shader) {
        glDeleteShader(geometry_shader);
    }

    ShaderProgramInfo created_shader_info{shader_program};

    created_shaders.insert({type, created_shader_info});
}

/**
 * \todo make sure that the vertex attribute variable is actually used in the selected shader type
 *
 * \brief configures a VAO so that it knows how to transmit data from a VBO into the shader program
 *
 * \note this assumes that each vertex attribute data array has its own unique vector for storing (usually a
 * std::vector<glm::vec3>>) [or whatever type of vector]
 *
 * \details a good way to think about this is that a shader program has a bunch of incoming conveyor belts, and we need
 * to tell the shader program how big the boxes that are coming in are for each different conveyor belt, then this
 * function is like a conveyor belt configuration, so that we say "BIG_BOXES" and then this function automates setting
 * up the conveyor belt so that the shader program can properly consume those boxes,
 *
 * With this analogy VAOs are like employees in charge of physical objects, they have different types of objects, say
 * toys, and books which are represented by VBOs they for each type of object, they need to configure how the
 * conveyor belt will operate for each object type (ie, the books can be packed closer than the toys).
 *
 * This function is then like a "training session" for the employee so that they learn how to configure the conveyor
 * belts, for their own custom objects this is done so that no matter what objects need to be packaged up, the
 * corresponding employee will know how to properly do this.
 *
 * \param vertex_attribute_object the vao that we need to configure the vertex attributes for
 * \param vertex_buffer_object the buffer contiaining the vertex attribute data
 * \param type the shader we want to do this for
 */
void ShaderCache::configure_vertex_attributes_for_drawables_vao(
    GLuint vertex_attribute_object, GLuint vertex_buffer_object, ShaderType type,
    ShaderVertexAttributeVariable shader_vertex_attribute_variable) {

    ShaderProgramInfo shader_program_info = get_shader_program(type);
    //    std::vector<ShaderVertexAttributeVariable> used_vertex_attributes_for_shader =
    //        get_used_vertex_attribute_variables_for_shader(type);

    glBindVertexArray(vertex_attribute_object); // enable the objects VAO
    // this has to occur because glEnableVertexArray and glVertexAttribPointer need to know about two things
    // the output and input we are configuring the communication between.
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);

    GLVertexAttributeConfiguration config =
        get_gl_vertex_attribute_configuration_for_vertex_attribute_variable(shader_vertex_attribute_variable);
    std::string svav_name = get_vertex_attribute_variable_name(shader_vertex_attribute_variable);

    if (logger_component.logging_enabled) {
        logger_component.get_logger()->info("Binding vertex attribute {}", svav_name);
    }

    GLuint vertex_attribute_location = glGetAttribLocation(shader_program_info.id, svav_name.c_str());
    glEnableVertexAttribArray(vertex_attribute_location);

    // When stride is 0, it tells OpenGL that the attributes are tightly packed in the array. That
    // means the next set of attribute data starts immediately after the current one. In this case, since the
    // data is tightly packed, OpenGL calculates the stride as 2 * sizeof(GL_FLOAT). This value is equivalent to
    // the size of the attribute data (in bytes) because the next attribute in the buffer starts right after the
    // previous one without any additional padding or interleaved data.
    glVertexAttribPointer(vertex_attribute_location, config.components_per_vertex, config.data_type_of_component,
                          config.normalize, config.stride, config.pointer_to_start_of_data);

    glBindVertexArray(0);
}

std::string ShaderCache::get_uniform_name(ShaderUniformVariable uniform) const {
    auto it = shader_standard.shader_uniform_variable_to_name.find(uniform);
    if (it != shader_standard.shader_uniform_variable_to_name.end()) {
        return it->second;
    }

    if (logger_component.logging_enabled) {
        logger_component.get_logger()->warn("Uniform variable enum {} not found in allowed names.",
                                            static_cast<int>(uniform));
    }
    return "";
}

GLVertexAttributeConfiguration ShaderCache::get_gl_vertex_attribute_configuration_for_vertex_attribute_variable(
    ShaderVertexAttributeVariable shader_vertex_attribute_variable) const {
    try {
        return shader_standard.shader_vertex_attribute_to_glva_configuration.at(shader_vertex_attribute_variable);
    } catch (const std::out_of_range &e) {

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error(
                "The specified shader vertex attribute variable doesn't have a gl vertex attribute configuration: {}",
                e.what());
        }
        throw;
    }
}

std::vector<ShaderVertexAttributeVariable>
ShaderCache::get_used_vertex_attribute_variables_for_shader(ShaderType type) const {
    try {
        return shader_standard.shader_to_used_vertex_attribute_variables.at(type);
    } catch (const std::out_of_range &e) {

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error(
                "The specified shader type doesn't have have any vertex attribute variables, please add some: {}",
                e.what());
        }
        throw;
    }
}

std::string
ShaderCache::get_vertex_attribute_variable_name(ShaderVertexAttributeVariable shader_vertex_attribute_variable) const {
    try {
        // Use `at` to access the name directly
        return shader_standard.shader_vertex_attribute_variable_to_name.at(shader_vertex_attribute_variable);
    } catch (const std::out_of_range &e) {
        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error(
                "The specified vertex attribute variable doesn't have a name in the mapping: {}", e.what());
        }
        // Rethrow the exception to continue propagating it upwards
        throw;
    }
}

GLint ShaderCache::get_uniform_location(ShaderType type, ShaderUniformVariable uniform) const {
    ShaderProgramInfo shader_info = get_shader_program(type);
    GLint location = glGetUniformLocation(shader_info.id, get_uniform_name(uniform).c_str());
    if (location == -1) {

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error("Uniform '{}' not found in shader program.",
                                                 get_uniform_name(uniform));
        }
    }
    return location;
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, bool value) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform1i(location, static_cast<int>(value));
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, int value) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, float value) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec2 &vec) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform2fv(location, 1, &vec[0]);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec3 &vec) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform3fv(location, 1, &vec[0]);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec4 &vec) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform4fv(location, 1, &vec[0]);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z, float w) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat2 &mat) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat3 &mat) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
    }
}

void ShaderCache::set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat4 &mat) {
    use_shader_program(type);
    GLint location = get_uniform_location(type, uniform);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
    }
}

GLuint ShaderCache::attach_shader(GLuint program, const std::string &path, GLenum shader_type) {
    std::string shader_code;
    std::ifstream shader_file;

    shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        shader_file.open(path);
        std::stringstream shader_stream;
        shader_stream << shader_file.rdbuf();
        shader_file.close();

        shader_code = shader_stream.str();
    } catch (const std::ifstream::failure &e) {

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ {}: {}", path, e.what());
        }
    }

    GLuint shader = glCreateShader(shader_type);
    const char *shader_code_ptr = shader_code.c_str();
    glShaderSource(shader, 1, &shader_code_ptr, nullptr);
    glCompileShader(shader);

    GLint success;
    GLchar info_log[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, info_log);

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error("ERROR::SHADER::COMPILATION_FAILED {}: {}", path, info_log);
        }
    }

    glAttachShader(program, shader);
    return shader;
}

void ShaderCache::link_program(GLuint program) {
    glLinkProgram(program);

    GLint success;
    GLchar info_log[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, info_log);

        if (logger_component.logging_enabled) {
            logger_component.get_logger()->error("ERROR::PROGRAM::LINKING_FAILED: {}", info_log);
        }
    }
}

void ShaderCache::log_shader_program_info() const {

    if (logger_component.logging_enabled) {
        logger_component.get_logger()->info("Logging Created Shaders:");
        logger_component.get_logger()->info("Total shaders: {}", created_shaders.size());
    }

    for (const auto &[shader_type, shader_info] : created_shaders) {
        if (logger_component.logging_enabled) {
            logger_component.get_logger()->info("Shader Type: {}, Program ID: {}",
                                                shader_standard.shader_type_to_name.at(shader_type), shader_info.id);
        }
    }
}
