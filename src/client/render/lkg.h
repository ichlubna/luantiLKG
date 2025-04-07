// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "stereo.h"

class LoadUniformsStep : public TrivialRenderStep
{
public:
	LoadUniformsStep(TextureBuffer *buffer, u8 index, std::vector<float> values);
	void run(PipelineContext &context);
private:
    TextureBuffer *buffer;
	u8 index;
    std::vector<float> values;
};

void populateLkgPipeline(RenderPipeline *pipeline, Client *client, bool horizontal, bool flipped, v2f &virtual_size_scale);
