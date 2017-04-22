/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "gl.h"

#include <mgba-util/math.h>

static const GLint _glTexCoords[] = {
	0, 0,
	1, 0,
	1, 1,
	0, 1
};

static void mGLContextInit(struct VideoBackend* v, WHandle handle) {
	UNUSED(handle);
	struct mGLContext* context = (struct mGLContext*) v;
	glGenTextures(1, &context->tex);
	glBindTexture(GL_TEXTURE_2D, context->tex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#ifndef _WIN32
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
}

static void mGLContextSetDimensions(struct VideoBackend* v, unsigned width, unsigned height) {
	struct mGLContext* context = (struct mGLContext*) v;
	v->width = width;
	v->height = height;

	glBindTexture(GL_TEXTURE_2D, context->tex);
#ifdef COLOR_16_BIT
#ifdef COLOR_5_6_5
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, toPow2(width), toPow2(height), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, toPow2(width), toPow2(height), 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, 0);
#endif
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, toPow2(width), toPow2(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#endif

	context->vertices[0] = 0;
	context->vertices[1] = 0;
	context->vertices[2] = toPow2(width);
	context->vertices[3] = 0;
	context->vertices[4] = toPow2(width);
	context->vertices[5] = toPow2(height);
	context->vertices[6] = 0;
	context->vertices[7] = toPow2(height);
}

static void mGLContextDeinit(struct VideoBackend* v) {
	struct mGLContext* context = (struct mGLContext*) v;
	glDeleteTextures(1, &context->tex);
}

static void mGLContextResized(struct VideoBackend* v, unsigned w, unsigned h) {
	unsigned drawW = w;
	unsigned drawH = h;
	if (v->lockAspectRatio) {
		if (w * v->height > h * v->width) {
			drawW = h * v->width / v->height;
		} else if (w * v->height < h * v->width) {
			drawH = w * v->height / v->width;
		}
	}
	if (v->lockIntegerScaling) {
		drawW -= drawW % v->width;
		drawH -= drawH % v->height;
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport((w - drawW) / 2, (h - drawH) / 2, drawW, drawH);
}

static void mGLContextClear(struct VideoBackend* v) {
	UNUSED(v);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void mGLContextDrawFrame(struct VideoBackend* v) {
	struct mGLContext* context = (struct mGLContext*) v;
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_INT, 0, context->vertices);
	glTexCoordPointer(2, GL_INT, 0, _glTexCoords);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, v->width, v->height, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, context->tex);
	if (v->filter) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void mGLContextPostFrame(struct VideoBackend* v, const void* frame) {
	struct mGLContext* context = (struct mGLContext*) v;
	glBindTexture(GL_TEXTURE_2D, context->tex);
#ifdef COLOR_16_BIT
#ifdef COLOR_5_6_5
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, v->width, v->height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, frame);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, v->width, v->height, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, frame);
#endif
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, v->width, v->height,  GL_RGBA, GL_UNSIGNED_BYTE, frame);
#endif
}

void mGLContextCreate(struct mGLContext* context) {
	context->d.init = mGLContextInit;
	context->d.deinit = mGLContextDeinit;
	context->d.setDimensions = mGLContextSetDimensions;
	context->d.resized = mGLContextResized;
	context->d.swap = 0;
	context->d.clear = mGLContextClear;
	context->d.postFrame = mGLContextPostFrame;
	context->d.drawFrame = mGLContextDrawFrame;
	context->d.setMessage = 0;
	context->d.clearMessage = 0;
}
