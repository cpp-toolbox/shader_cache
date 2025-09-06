#ifndef SHADER_CACHE_HPP
#define SHADER_CACHE_HPP

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "sbpt_generated_includes.hpp"

/**
 * \brief facilitates simple and robust interaction with shaders
 *
 * \details a shader cache is a tool which helps manage shaders in a sane way,
 * firstly the user defines what types of shaders by passing in the requested
 * shaders from the shader catalog, then that shader can be selected easily, it
 * also adheres to the cpp_toolbox standard for naming variables in shader
 * files, this makes it easy to read shader files as when you see a variable you
 * will know exactly what it standard for. With these features it makes it hard
 * to mess up when using shaders
 *
 */
class ShaderCache {
  public:
    ShaderCache(std::vector<ShaderType> requested_shaders);
    ~ShaderCache();
    ShaderStandard shader_standard;

    Logger logger = Logger("shader_cache");

    ShaderProgramInfo get_shader_program(ShaderType type) const;
    void use_shader_program(ShaderType type);
    void stop_using_shader_program();
    void register_shader_program(ShaderType type);

    void configure_vertex_attributes_for_drawables_vao(GLuint vertex_attribute_object, GLuint vertex_buffer_object,
                                                       ShaderType type,
                                                       ShaderVertexAttributeVariable shader_vertex_attribute_variable);

    void log_shader_program_info();

    GLVertexAttributeConfiguration get_gl_vertex_attribute_configuration_for_vertex_attribute_variable(
        ShaderVertexAttributeVariable shader_vertex_attribute_variable);
    std::vector<ShaderVertexAttributeVariable> get_used_vertex_attribute_variables_for_shader(ShaderType type);
    std::string get_vertex_attribute_variable_name(ShaderVertexAttributeVariable shader_vertex_attribute_variable);
    std::string get_uniform_name(ShaderUniformVariable uniform);
    GLint get_uniform_location(ShaderType type, ShaderUniformVariable uniform);

    void set_uniform(ShaderType type, ShaderUniformVariable uniform, bool value);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, int value);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float value);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec2 &vec);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec3 &vec);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec4 &vec);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const std::vector<glm::vec4> &values);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z, float w);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat2 &mat);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat3 &mat);
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat4 &mat);
    void print_out_active_uniforms_in_shader(ShaderType type);

  private:
    GLuint attach_shader(GLuint program, const std::string &path, GLenum shader_type);
    void link_program(GLuint program);

    std::unordered_map<ShaderType, ShaderProgramInfo> created_shaders;
};

std::string shader_type_to_string(ShaderType type);

#endif // SHADER_CACHE_HPP
