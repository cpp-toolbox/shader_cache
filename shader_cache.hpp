#ifndef SHADER_CACHE_HPP
#define SHADER_CACHE_HPP

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <glad/glad.h>
#include "spdlog/spdlog.h"

enum class ShaderType {
    CWL_V_TRANSFORMATION_WITH_SOLID_COLOR,
    CWL_V_TRANSFORMATION_WITH_TEXTURES,
    CWL_V_TRANSFORMATION_WITH_TEXTURES_AMBIENT_LIGHTING,
    CWL_V_TRANSFORMATION_WITH_TEXTURES_AMBIENT_AND_DIFFUSE_LIGHTING,
    SKYBOX,
    ABSOLUTE_POSITION_WITH_SOLID_COLOR,
    TEXT,
};

enum class ShaderVertexAttributeVariable {
    POSITION,
    XY_POSITION,
    PASSTHROUGH_TEXTURE_COORDINATE,
    PASSTHROUGH_NORMAL,
};

enum class ShaderUniformVariable {
    // Transformations
    CAMERA_TO_CLIP, // mat4
    WORLD_TO_CAMERA, // mat4
    LOCAL_TO_WORLD, // mat4
    // Textures
    SKYBOX_TEXTURE_UNIT,
    TEXT_TEXTURE_UNIT, 
    COLOR, // vec3 (should be removed eventually)
    RGB_COLOR, // vec3
    RGBA_COLOR, // vec4
    // Lighting 
    AMBIENT_LIGHT_STRENGTH, // float
    AMBIENT_LIGHT_COLOR, // vec3
    DIFFUSE_LIGHT_POSITION, // vec3
};

struct ShaderCreationInfo {
    std::string vertex_path;
    std::string fragment_path;
    std::string geometry_path;
};

struct ShaderProgramInfo {
    GLuint id;
    //    // the location of the vertex attribute in the source file
    //    std::unordered_map<ShaderVertexAttributeVariable, GLint> vertex_attribute_to_location;
};

struct GLVertexAttributeConfiguration {
    GLint components_per_vertex;
    GLenum data_type_of_component;
    GLboolean normalize;
    GLsizei stride;
    GLvoid *pointer_to_start_of_data;
};

/**
 * \brief facilitates simple and robust interaction with shaders
 *
 * \details a shader cache is a tool which helps manage shaders in a sane way, firstly the user defines what types of
 * shaders by passing in the requested shaders from the shader catalog, then that shader can be selected easily, it also
 * adheres to the cpp_toolbox standard for naming variables in shader files, this makes it easy to read shader files as
 * when you see a variable you will know exactly what it standard for. With these features it makes it hard to mess up
 * when using shaders
 *
 * \usage See how it it used in DiffuseTexturedDiv and BaseDiv
 *
 */
class ShaderCache {
  public:
    ShaderCache(std::vector<ShaderType> requested_shaders);
    ~ShaderCache();

    ShaderProgramInfo get_shader_program(ShaderType type) const;
    void use_shader_program(ShaderType type);
    void stop_using_shader_program();
    void create_shader_program(ShaderType type);

    void configure_vertex_attributes_for_drawables_vao(GLuint vertex_attribute_object, GLuint vertex_buffer_object,
                                                       ShaderType type,
                                                       ShaderVertexAttributeVariable shader_vertex_attribute_variable);

    void log_shader_program_info() const;

    GLVertexAttributeConfiguration get_gl_vertex_attribute_configuration_for_vertex_attribute_variable(
        ShaderVertexAttributeVariable shader_vertex_attribute_variable) const;
    std::vector<ShaderVertexAttributeVariable> get_used_vertex_attribute_variables_for_shader(ShaderType type) const;
    std::string
    get_vertex_attribute_variable_name(ShaderVertexAttributeVariable shader_vertex_attribute_variable) const;
    std::string get_uniform_name(ShaderUniformVariable uniform) const;
    GLint get_uniform_location(ShaderType type, ShaderUniformVariable uniform) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, bool value) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, int value) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float value) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec2 &vec) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec3 &vec) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::vec4 &vec) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, float x, float y, float z, float w) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat2 &mat) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat3 &mat) const;
    void set_uniform(ShaderType type, ShaderUniformVariable uniform, const glm::mat4 &mat) const;

  private:
    GLuint attach_shader(GLuint program, const std::string &path, GLenum shader_type);
    void link_program(GLuint program);

    std::unordered_map<ShaderType, ShaderProgramInfo> created_shaders;

    // TODO find a better place for these...

    /// This assumes that you use a std::vector<glm::Vec[2,3,4]> for storing data
    const std::unordered_map<ShaderVertexAttributeVariable, GLVertexAttributeConfiguration>
        shader_vertex_attribute_to_glva_configuration = {
            {ShaderVertexAttributeVariable::POSITION, {3, GL_FLOAT, GL_FALSE, 0, (void *)0}},
            {ShaderVertexAttributeVariable::XY_POSITION, {2, GL_FLOAT, GL_FALSE, 0, (void *)0}},
            {ShaderVertexAttributeVariable::PASSTHROUGH_NORMAL, {3, GL_FLOAT, GL_FALSE, 0, (void *)0}},
            {ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE, {2, GL_FLOAT, GL_FALSE, 0, (void *)0}},
    };

    const std::unordered_map<ShaderUniformVariable, std::string> shader_uniform_variable_to_name = {
        // Transformations
        {ShaderUniformVariable::CAMERA_TO_CLIP, "camera_to_clip"},
        {ShaderUniformVariable::WORLD_TO_CAMERA, "world_to_camera"},
        {ShaderUniformVariable::LOCAL_TO_WORLD, "local_to_world"},
        // Textures
        {ShaderUniformVariable::SKYBOX_TEXTURE_UNIT, "skybox_texture_unit"},
        {ShaderUniformVariable::TEXT_TEXTURE_UNIT, "text_texture_unit"},
        {ShaderUniformVariable::COLOR, "color"}, // will be deprecated for rgba_color
        {ShaderUniformVariable::RGB_COLOR, "rgb_color"},
        {ShaderUniformVariable::RGBA_COLOR, "rgba_color"},
        // Lighting
        {ShaderUniformVariable::AMBIENT_LIGHT_COLOR , "ambient_light_color"},
        {ShaderUniformVariable::AMBIENT_LIGHT_STRENGTH , "ambient_light_strength"},
        {ShaderUniformVariable::DIFFUSE_LIGHT_POSITION , "diffuse_light_position"},
    };

    std::unordered_map<ShaderType, ShaderCreationInfo> shader_catalog = {
        {ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR,
         {"assets/shaders/CWL_v_transformation.vert", "assets/shaders/solid_color.frag"}},
        {ShaderType::CWL_V_TRANSFORMATION_WITH_TEXTURES,
         {"assets/shaders/CWL_v_transformation_with_texture_coordinate_passthrough.vert",
          "assets/shaders/textured.frag"}},
        {ShaderType::CWL_V_TRANSFORMATION_WITH_TEXTURES_AMBIENT_LIGHTING,
         {"assets/shaders/CWL_v_transformation_with_texture_coordinate_passthrough.vert",
          "assets/shaders/textured_with_ambient_lighting.frag"}},
        {ShaderType::CWL_V_TRANSFORMATION_WITH_TEXTURES_AMBIENT_AND_DIFFUSE_LIGHTING,
         {"assets/shaders/CWL_v_transformation_with_texture_coordinate_and_normal_passthrough.vert",
          "assets/shaders/textured_with_ambient_and_diffuse_lighting.frag"}},
        {ShaderType::SKYBOX, {"assets/shaders/cubemap.vert", "assets/shaders/cubemap.frag"}},
        {
            ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR,
            {"assets/shaders/absolute_position.vert", "assets/shaders/solid_color.frag"},
        },
        {
            ShaderType::TEXT,
            {"assets/shaders/text.vert", "assets/shaders/text.frag"},
        },
        {
            ShaderType::TEXT,
            {"assets/shaders/text.vert", "assets/shaders/text.frag"},
        },
    };

    // TODO: This should probably be automated at some point by reading the file and checking for the vars automatically also make one of these for the uniforms as well
    std::unordered_map<ShaderType, std::vector<ShaderVertexAttributeVariable>>
        shader_to_used_vertex_attribute_variables = {
            {ShaderType::CWL_V_TRANSFORMATION_WITH_TEXTURES,
             {ShaderVertexAttributeVariable::POSITION, ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE}},
            {ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR, {ShaderVertexAttributeVariable::POSITION}},
            {ShaderType::SKYBOX, {ShaderVertexAttributeVariable::POSITION}},
            {ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, {ShaderVertexAttributeVariable::POSITION}},
            /* { */
            /*     ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, */
            /*     {ShaderVertexAttributeVariable::XY_POSITION, */
            /*      ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE}, */
            /* }, */
            {ShaderType::TEXT,
             {ShaderVertexAttributeVariable::XY_POSITION,
              ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE}},
    };

    const std::unordered_map<ShaderVertexAttributeVariable, std::string> shader_vertex_attribute_variable_to_name = {
        {ShaderVertexAttributeVariable::POSITION, "position"},
        {ShaderVertexAttributeVariable::XY_POSITION, "xy_position"},
        {ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE, "passthrough_texture_coordinate"},
        {ShaderVertexAttributeVariable::PASSTHROUGH_NORMAL, "passthrough_normal"},
    };
};

#endif // SHADER_CACHE_HPP
