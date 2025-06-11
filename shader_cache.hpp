#ifndef SHADER_CACHE_HPP
#define SHADER_CACHE_HPP

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <glad/glad.h>
#include "sbpt_generated_includes.hpp"

/*
    LogLevel Usage in Game Engine Context:

    LogLevel::Trace
    - Extremely detailed, fine-grained logs.
    - Used for tracing code execution paths, function entry/exit, memory ops, ECS steps, etc.
    - Typically disabled in production due to volume/performance cost.

    LogLevel::Debug
    - General-purpose debugging information.
    - Used to track game state, system behavior, asset loading, AI decisions, input handling, etc.
    - Useful during active development; not enabled in production builds.

    LogLevel::Info
    - High-level runtime information indicating normal operation.
    - Examples: level loaded, player joined, shaders compiled, assets hot-reloaded.
    - Suitable for both development and production for visibility into system flow.

    LogLevel::Warn
    - Indicates a potential issue or unexpected condition that is non-fatal.
    - Examples: missing optional assets, deprecated API use, high frame times, fallback behavior.
    - Deserves attention, useful for QA, testing, and sometimes user-visible logs.

    LogLevel::Error
    - A serious issue that impacted functionality but did not crash the engine.
    - Examples: asset load failures, script exceptions, physics errors, network disconnects.
    - Should be logged prominently and often triggers error-handling logic.

    LogLevel::Critical
    - Fatal errors that prevent continued execution or indicate corruption/system failure.
    - Examples: GPU device failure, save data corruption, out-of-memory, engine init failure.
    - Typically results in shutdown, crash, or forced failover.
*/

#include <string_view>
#include <fmt/core.h>

enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical };

class ILogger {
  public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, std::string_view message) = 0;

    template <typename... Args> void trace(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Trace, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args> void debug(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Debug, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args> void info(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Info, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args> void warn(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Warn, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args> void error(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Error, fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args> void critical(fmt::format_string<Args...> fmt_str, Args &&...args) {
        log(LogLevel::Critical, fmt::format(fmt_str, std::forward<Args>(args)...));
    }
};

#include <iostream>

class ConsoleLogger : public ILogger {
  public:
    void log(LogLevel level, std::string_view message) override {
        std::cout << "[" << level_to_string(level) << "] " << message << std::endl;
    }

  private:
    const char *level_to_string(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:
            return "Trace";
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Warn:
            return "Warn";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Critical:
            return "Critical";
        }
        return "Unknown";
    }
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
 */
class ShaderCache {
  public:
    ShaderCache(std::vector<ShaderType> requested_shaders);
    ~ShaderCache();
    ShaderStandard shader_standard;

    ConsoleLogger console_logger;

    ShaderProgramInfo get_shader_program(ShaderType type) const;
    void use_shader_program(ShaderType type);
    void stop_using_shader_program();
    void create_shader_program(ShaderType type);

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
