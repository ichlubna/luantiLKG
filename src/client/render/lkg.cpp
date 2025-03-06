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

DrawImageStepLkg::DrawImageStepLkg(u8 texture_index, v2f _offset) :
	texture_index(texture_index), offset(_offset)
{}

void DrawImageStepLkg::setRenderSource(RenderSource *_source)
{
	source = _source;
}
void DrawImageStepLkg::setRenderTarget(RenderTarget *_target)
{
	target = _target;
}

void DrawImageStepLkg::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	auto texture = source->getTexture(texture_index);
	core::dimension2du output_size = context.device->getVideoDriver()->getScreenSize();
	v2s32 pos(offset.X * output_size.Width, offset.Y * output_size.Height);
	
	//context.client->getCamera()->getCameraNode()->setPosition({0.0f, 20.0f, 0.0f});
	
    //auto transform = context.device->getVideoDriver()->getTransform(irr::video::ETS_VIEW);
    //transform.setTranslation({0.0f, 20.0f, 0.0f});
	//context.device->getVideoDriver()->setTransform(irr::video::ETS_VIEW, transform);
    context.client->getCamera()->m_camera_position = { 0.0f, 20.0f, 0.0f }; 

	context.device->getVideoDriver()->draw2DImage(texture, pos);
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

	auto driver = client->getSceneManager()->getVideoDriver();
	video::ECOLOR_FORMAT color_format = selectColorFormat(driver);
	video::ECOLOR_FORMAT depth_format = selectDepthFormat(driver);

	v2f offset;
    virtual_size_scale = v2f(1.0f/holo.params["Cols"], 1.0f/holo.params["Rows"]);

	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
    for(size_t i = 0; i < viewCount; i++) 
    	buffer->setTexture(i, virtual_size_scale, "3d_render_" + std::to_string(i), color_format);

    buffer->setTexture(TEXTURE_DEPTH, virtual_size_scale, "3d_depthmap", depth_format);

	auto step3D = pipeline->own(create3DStage(client, virtual_size_scale));

	for (size_t i = 0; i < viewCount; i++) 
    {
		pipeline->addStep<OffsetCameraStep>(i, viewCount, holo.params["CameraSpacingStep"], holo.params["FocusSpacingStep"]);
		auto output = pipeline->createOwned<TextureBufferOutput>(
				buffer, std::vector<u8>{static_cast<u8>(i)}, TEXTURE_DEPTH);
		pipeline->addStep<SetRenderTargetStep>(step3D, output);
		pipeline->addStep(step3D);
		pipeline->addStep<DrawWield>();
		pipeline->addStep<MapPostFxStep>();
		pipeline->addStep<DrawHUD>();
	}

	pipeline->addStep<OffsetCameraStep>(0.0f);

	auto screen = pipeline->createOwned<ScreenTarget>();

	for (size_t i = 0; i < viewCount; i++)
    {
        size_t x = i % static_cast<size_t>(holo.params["Cols"]);
        size_t y = i / static_cast<size_t>(holo.params["Cols"]);
        offset = v2f(virtual_size_scale[0] * x, virtual_size_scale[1] * y); 
		auto step = pipeline->addStep<DrawImageStepLkg>(i, offset);
		step->setRenderSource(buffer);
		step->setRenderTarget(screen);
	}
}
