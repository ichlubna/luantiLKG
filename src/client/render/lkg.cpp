// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "lkg.h"
#include "client/client.h"
#include "client/hud.h"
#include "client/camera.h"
#include <ISceneManager.h>
#include <fstream>
#include "client/shader.h"
#include "secondstage.h"

LoadUniformsStep::LoadUniformsStep(TextureBuffer *_buffer, u8 _index, std::vector<float> _values) :	buffer(_buffer), index(_index), values(_values)
{}

void LoadUniformsStep::run(PipelineContext &context)
{
    auto texture = buffer->getTexture(index);
	float *data = reinterpret_cast<float*>(texture->lock(video::ETLM_WRITE_ONLY));
    for(size_t i=0; i<values.size(); i++)
        data[i] = values[i];
    texture->unlock();
}

class HoloSettings
{
public: 
    std::map<std::string, float> params;

    HoloSettings(std::string fileName)
    {
        std::ifstream file(fileName);
        std::string line;
        while (std::getline(file, line))
            {
            std::vector<std::string> tokens = split(line, '=');
            params[tokens[0]] = mystof(tokens[1]);
        }
    }
};

void populateLkgPipeline(RenderPipeline *pipeline, Client *client, bool horizontal, bool flipped, v2f &virtual_size_scale)
{
    HoloSettings holo("holo.conf");
    client->holoCameraSpacing = holo.params["InitCameraSpacing"];
    client->holoCameraFocus = holo.params["InitFocusSpacing"];
    size_t viewCount = static_cast<size_t>(holo.params["Rows"]*holo.params["Cols"]);

	static const u8 TEXTURE_DEPTH = viewCount;
    static const u8 TEXTURE_UNIFORM = viewCount + 1;
    static const u8 TEXTURE_TEMP = viewCount + 2;

	auto driver = client->getSceneManager()->getVideoDriver();
	video::ECOLOR_FORMAT color_format = selectColorFormat(driver);
	video::ECOLOR_FORMAT depth_format = selectDepthFormat(driver);

	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
    virtual_size_scale = v2f(1.0f, 1.0f);
    buffer->setTexture(TEXTURE_DEPTH, virtual_size_scale, "3d_depthmap", depth_format);
    buffer->setTexture(TEXTURE_UNIFORM, v2f(0.01f, 0.01f), "3d_uniform", video::ECF_R32F);
    buffer->setTexture(TEXTURE_TEMP, virtual_size_scale, "3d_render_temp", color_format);
   
    auto step3D = pipeline->own(create3DStage(client, virtual_size_scale));
    std::vector<u8> textureIDs;
	for (size_t i = 0; i < viewCount; i++) 
    {
    	buffer->setTexture(i, virtual_size_scale, "3d_render_" + std::to_string(i), color_format);
        textureIDs.push_back(i);

		pipeline->addStep<OffsetCameraStep>(i, viewCount, holo.params["CameraSpacingStep"], holo.params["FocusSpacingStep"]);
		auto output = pipeline->createOwned<TextureBufferOutput>(buffer, std::vector<u8>{static_cast<u8>(i)}, TEXTURE_DEPTH);
		pipeline->addStep<SetRenderTargetStep>(step3D, output);
		pipeline->addStep(step3D);
		pipeline->addStep<DrawWield>();
		pipeline->addStep<MapPostFxStep>();
		pipeline->addStep<DrawHUD>();
	}
	pipeline->addStep<OffsetCameraStep>(0.0f);
 
    constexpr size_t UNIFORM_ITERATION_ID = 0; 
    constexpr size_t UNIFORM_MODE_ID = 1; 
    std::vector<float> shaderUniforms = {0.0f, 0.0f, holo.params["Tilt"], holo.params["Pitch"], holo.params["Center"], holo.params["ViewPortionElement"], holo.params["Subp"], static_cast<float>(viewCount), holo.params["Cols"], holo.params["Rows"]};
    uint32_t shader_id = client->getShaderSource()->getShaderRaw("filter");
    shaderUniforms[UNIFORM_MODE_ID] = 0;
    auto output = pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_TEMP);
    for(size_t i=0; i<viewCount; i++) 
    {
        shaderUniforms[UNIFORM_ITERATION_ID] = viewCount-i-1;
        pipeline->addStep<LoadUniformsStep>(buffer, TEXTURE_UNIFORM, shaderUniforms); 
        PostProcessingStep *effect = pipeline->createOwned<PostProcessingStep>(shader_id, std::vector<u8>{static_cast<u8>(i), TEXTURE_UNIFORM});
        pipeline->addStep(effect);
        effect->setRenderSource(buffer);
        effect->setRenderTarget(output);
    }
           
    shaderUniforms[UNIFORM_MODE_ID] = 1;
    if(static_cast<int>(holo.params["Quilt"]) == 1) 
        shaderUniforms[UNIFORM_MODE_ID] = 2;
    pipeline->addStep<LoadUniformsStep>(buffer, TEXTURE_UNIFORM, shaderUniforms); 
	auto screen = pipeline->createOwned<ScreenTarget>();
    PostProcessingStep *holoEffect = pipeline->createOwned<PostProcessingStep>(shader_id, std::vector<u8>{TEXTURE_TEMP, TEXTURE_UNIFORM});
    pipeline->addStep(holoEffect);
    holoEffect->setRenderSource(buffer);
    holoEffect->setRenderTarget(screen);
}
