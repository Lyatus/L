#include <L/src/stream/CFileStream.h>
#include <L/src/engine/Resource.h>
#include <L/src/rendering/Shader.h>
#include <L/src/rendering/shader_lib.h>

using namespace L;

void glsl_loader(Resource<Shader>::Slot& slot) {
  VkShaderStageFlagBits stage(VK_SHADER_STAGE_ALL);
  String stage_name;
  if(!strcmp(strrchr(slot.path, '.'), ".frag")) {
    stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage_name = "frag";
  } else if(!strcmp(strrchr(slot.path, '.'), ".vert")) {
    stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage_name = "vert";
  }

  String cmd_output;

  {
    char input_file[L_tmpnam], output_file[L_tmpnam];
    tmpnam(input_file);
    tmpnam(output_file);

    {
      Buffer original_text(CFileStream(slot.path, "rb").read_into_buffer());
      CFileStream input_stream(input_file, "wb");
      input_stream << L_GLSL_INTRO << L_SHAREDUNIFORM << L_PUSH_CONSTANTS;
      if(stage == VK_SHADER_STAGE_FRAGMENT_BIT)
        input_stream << L_SHADER_LIB;
      input_stream << '\n';
      input_stream.write(original_text.data(), original_text.size());
    }

    const String cmd("glslangValidator -V -q -S "+stage_name+" "+input_file+" -o "+output_file);
    System::call(cmd, cmd_output);

    {
      CFileStream output_stream(output_file, "rb");
      slot.value = ref<Shader>(output_stream.read_into_buffer(), stage);
    }
  }

  if(Ref<Shader> shader = slot.value) { // Parse debug information
    Shader::BindingType binding_type(Shader::BindingType::None);
    Array<String> lines(cmd_output.explode('\n')), words;
    for(String& line : lines) {
      if(line=="Uniform reflection:")
        binding_type = Shader::BindingType::Uniform;
      else if(line=="Uniform block reflection:")
        binding_type = Shader::BindingType::UniformBlock;
      else if(line=="Vertex attribute reflection:")
        binding_type = Shader::BindingType::VertexAttribute;
      else if(binding_type!=Shader::BindingType::None) {
        line.replaceAll(",", "").replaceAll(":", "");
        words = line.explode(' ');
        Shader::Binding binding;
        binding.name = Symbol(words[0]);
        binding.offset = atoi(words[2]);
        binding.type = binding_type;
        binding.size = atoi(words[6]);
        binding.index = atoi(words[8]);
        binding.binding = atoi(words[10]);

        // FIXME: this is a hack until we have working reflection for shader vertex attributes
        if(binding.name==Symbol("vposition")) {
          binding.binding = 0;
          binding.index = 0;
          binding.format = VK_FORMAT_R32G32B32_SFLOAT;
          binding.offset = 0;
          binding.size = 12;
        }
        if(binding.name==Symbol("vtexcoords")) {
          binding.binding = 0;
          binding.index = 1;
          binding.format = VK_FORMAT_R32G32_SFLOAT;
          binding.offset = 12;
          binding.size = 8;
        }
        if(binding.name==Symbol("vnormal")) {
          binding.binding = 0;
          binding.index = 2;
          binding.format = VK_FORMAT_R32G32B32_SFLOAT;
          binding.offset = 20;
          binding.size = 12;
        }
        if(binding.name==Symbol("vpositionfont")) {
          binding.binding = 0;
          binding.index = 0;
          binding.format = VK_FORMAT_R32G32_SFLOAT;
          binding.offset = 0;
          binding.size = 8;
        }
        if(binding.name==Symbol("vtexcoordsfont")) {
          binding.binding = 0;
          binding.index = 1;
          binding.format = VK_FORMAT_R32G32_SFLOAT;
          binding.offset = 8;
          binding.size = 8;
        }
        shader->add_binding(binding);
      }
    }
  }
}

void glsl_module_init() {
  Resource<Shader>::add_loader("vert", glsl_loader);
  Resource<Shader>::add_loader("frag", glsl_loader);
}