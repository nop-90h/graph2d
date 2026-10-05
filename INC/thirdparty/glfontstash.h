//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef GLFONTSTASH_H
#define GLFONTSTASH_H

#include "gfx.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

static FONScontext* glfonsCreate(int width, int height, int flags);
static void glfonsDelete(FONScontext* ctx);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

struct GLFONScontext {
	g2d::CTexturePtr ptrTex;
	FONScontext* p;
};
typedef struct GLFONScontext GLFONScontext;

static int glfons__renderCreate(void* userPtr, int width, int height)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;


	// Create may be called multiple times, delete existing texture.
	if (gl->ptrTex) 
	{
		g2d::CGfx::getInstance()->deleteTexture(gl->ptrTex);
		gl->ptrTex = nullptr;
	}

	gl->ptrTex = g2d::CGfx::getInstance()->uploadFontStahTexture(width, height);
	return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height)
{
	// Reuse create to resize too.
	return glfons__renderCreate(userPtr, width, height);
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
	//static int nCounter = 0;
	GLFONScontext* gl = (GLFONScontext*)userPtr;

	if (!gl->ptrTex) 
		return;

	g2d::CGfx::getInstance()->updateFontStashTexture(gl->ptrTex, rect, data);

	int cx, cy;

    const unsigned char* fontTexData = fonsGetTextureData(gl->p, &cx, &cy);

    //if (!stbi_write_png(std::format("./fuck{}.png", nCounter).c_str(), cx, cy, 1, fontTexData, cx)) {
    //    assert(false);
   // }
	//nCounter++;
    
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	if (!gl->ptrTex) 
		return;
	g2d::CGfx::getInstance()->batchFontStashVerts(gl->ptrTex, verts, tcoords, colors, nverts);
}

static void glfons__renderDelete(void* userPtr)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	if (gl->ptrTex)
		g2d::CGfx::getInstance()->deleteTexture(gl->ptrTex);
	gl->ptrTex = nullptr;
	free(gl);
}


static FONScontext* glfonsCreate(int width, int height, int flags)
{
	FONSparams params;
	GLFONScontext* gl;

	gl = (GLFONScontext*)malloc(sizeof(GLFONScontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(GLFONScontext));

	memset(&params, 0, sizeof(params));
	params.width = width;
	params.height = height;
	params.flags = (unsigned char)flags;
	params.renderCreate = glfons__renderCreate;
	params.renderResize = glfons__renderResize;
	params.renderUpdate = glfons__renderUpdate;
	params.renderDraw = glfons__renderDraw; 
	params.renderDelete = glfons__renderDelete;
	params.userPtr = gl;
	gl->p = fonsCreateInternal(&params); 
	return gl->p;

error:
	if (gl != NULL) free(gl);
	return NULL;
}

static void glfonsDelete(FONScontext* ctx)
{
	fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (a) | (b << 8) | (g << 16) | (r << 24);
}

#endif
