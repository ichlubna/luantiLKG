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

DrawImageStepLkg::DrawImageStepLkg(TextureBuffer *_buffer, u8 _index, u8 _value) :	buffer(_buffer), index(_index), value(_value)
{}

void DrawImageStepLkg::run(PipelineContext &context)
{
    auto texture = buffer->getTexture(index);
	auto size = texture->getSize();
	u8 *data = reinterpret_cast<u8*>(texture->lock(video::ETLM_WRITE_ONLY));
    memset(data, value, size.Height * size.Width);
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
    size_t viewCount = static_cast<size_t>(holo.params["Rows"]*holo.params["Cols"]);

	static const u8 TEXTURE_DEPTH = viewCount;
    static const u8 TEXTURE_UNIFORM = viewCount + 1;

	auto driver = client->getSceneManager()->getVideoDriver();
	video::ECOLOR_FORMAT color_format = selectColorFormat(driver);
	video::ECOLOR_FORMAT depth_format = selectDepthFormat(driver);

	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
    virtual_size_scale = v2f(1.0f/holo.params["Cols"], 1.0f/holo.params["Rows"]);
    buffer->setTexture(TEXTURE_DEPTH, virtual_size_scale, "3d_depthmap", depth_format);
    buffer->setTexture(TEXTURE_UNIFORM, virtual_size_scale, "3d_uniform", video::ECF_A8R8G8B8);
    
    pipeline->addStep<DrawImageStepLkg>(buffer, TEXTURE_UNIFORM, 200); 
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
	auto screen = pipeline->createOwned<ScreenTarget>();
 
    constexpr size_t BATCH_SIZE = 3;
    size_t iterations = std::ceil(static_cast<float>(viewCount) / BATCH_SIZE);
    for (size_t i = 0; i < iterations; i++)
    {
        std::vector<u8> batch;
        for (size_t j = 0; j < BATCH_SIZE; j++)
        {
            size_t index = i * BATCH_SIZE + j;
            if (index < viewCount)
                batch.push_back(textureIDs[index]);  
            else
                batch.push_back(0);
        }  
        batch.push_back(TEXTURE_UNIFORM);

        uint32_t shader_id = client->getShaderSource()->getShaderRaw("filter");
        PostProcessingStep *effect = pipeline->createOwned<PostProcessingStep>(shader_id, batch);
        pipeline->addStep(effect);
        effect->setRenderSource(buffer);
        effect->setRenderTarget(screen);
    }
}
