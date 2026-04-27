#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "../constants.hpp"
#include "pugixml.hpp"



class TextureAtlas
{
public:
	inline bool load(SDL_Renderer *renderer, const std::string &imagePath, const std::string &xmlPath);
	inline void unload();
	inline const SDL_Rect* getRect(const std::string &name) const;
	inline void draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale = 1.0f, bool fliphoriz = false);
	inline void draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect);
	inline void drawSource(SDL_Renderer *renderer, const SDL_FRect *srcrect, const SDL_FRect *dstrect);
	inline void drawCover(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect);
	inline void drawTiled(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect, float scale = 1.0f);
	inline void drawFrame(SDL_Renderer *renderer, const std::string &name, int frameWidth, int frameHeight,
		int frameIndex, const SDL_FRect *dstrect, bool fliphoriz = false);
	inline void drawFrameSection(SDL_Renderer *renderer, const std::string &name, int frameWidth, int frameHeight,
		int frameIndex, const SDL_FRect *dstrect, float cropLeft, float cropTop, float cropRight,
		float cropBottom, bool fliphoriz = false);
	inline void drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, const SDL_FRect& dstrect);
	inline void drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, int x, int y);

private:
	SDL_Texture *texture = nullptr;
	std::unordered_map<std::string, SDL_Rect> atlas;
};

// Very minimal XML tag parser (enough for <SubTexture name="..." x="..." .../>)
static std::unordered_map<std::string, SDL_Rect> parseAtlasXML(const std::string &xmlPath)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
	if (!result)
		throw std::runtime_error("Failed to parse file: " + xmlPath);
	
	std::unordered_map<std::string, SDL_Rect> atlas;
	for (auto tex : doc.child("TextureAtlas").children("SubTexture"))
	{
		SDL_Rect rect;
		rect.x = tex.attribute("x").as_int();
		rect.y = tex.attribute("y").as_int();
		rect.w = tex.attribute("width").as_int();
		rect.h = tex.attribute("height").as_int();
		std::string name = tex.attribute("name").as_string();
		atlas[name] = rect;
	}

	return atlas;
}

inline bool TextureAtlas::load(SDL_Renderer *renderer, const std::string &imagePath, const std::string &xmlPath)
{
	// ⚠️ SDL3 cannot load PNGs by default — only BMP, unless SDL_image is added.
	SDL_Surface *surface = SDL_LoadBMP(imagePath.c_str());
	// SDL_Surface *surface = SDL_LoadBMP((std::filesystem::current_path().parent_path().string() + "/resources/assets/Atlas/atlas.bmp").c_str());
	if (!surface)
	{
		std::cerr << "Failed to load image: " << imagePath.c_str() << " — " << SDL_GetError() << "\n";
		return false;
	}

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface);

	if (!texture)
	{
		std::cerr << "Failed to create texture: " << SDL_GetError() << "\n";
		return false;
	}

	atlas = parseAtlasXML(xmlPath);
	if (atlas.empty())
	{
		std::cerr << "Warning: No sprites found in " << xmlPath << "\n";
	}

	SDL_SetTextureScaleMode(texture, SDL_ScaleMode::SDL_SCALEMODE_PIXELART);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

	return true;
}

inline void TextureAtlas::unload()
{
	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
	atlas.clear();
}

inline const SDL_Rect* TextureAtlas::getRect(const std::string &name) const
{
	auto it = atlas.find(name);
	return it == atlas.end() ? nullptr : &it->second;
}

inline void TextureAtlas::draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale, bool fliphoriz)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	const SDL_Rect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	float width = (float)r.w * scale;
	SDL_FRect dst = {fliphoriz ? (float)x - width : (float)x, (float)y, width, (float)r.h * scale};
	SDL_RenderTextureRotated(renderer, texture, &src, &dst, 0.0, nullptr,
		fliphoriz ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

inline void TextureAtlas::draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}
	const SDL_Rect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};

	// If dstrect is null, draw the sprite at its original size at (0,0)
	SDL_FRect default_dst = {0.0f, 0.0f, (float)r.w, (float)r.h};
	SDL_RenderTexture(renderer, texture, &src, dstrect ? dstrect : &default_dst);
}

inline void TextureAtlas::drawSource(SDL_Renderer *renderer, const SDL_FRect *srcrect, const SDL_FRect *dstrect)
{
	if (!texture || !srcrect)
		return;

	SDL_RenderTexture(renderer, texture, srcrect, dstrect);
}

inline void TextureAtlas::drawCover(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect)
{
	if (!dstrect || dstrect->w <= 0.0f || dstrect->h <= 0.0f)
		return;

	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	const SDL_Rect &r = it->second;
	float dstAspect = dstrect->w / dstrect->h;
	float srcAspect = static_cast<float>(r.w) / static_cast<float>(r.h);
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};

	if (srcAspect > dstAspect)
	{
		float cropWidth = static_cast<float>(r.h) * dstAspect;
		src.x += (static_cast<float>(r.w) - cropWidth) * 0.5f;
		src.w = cropWidth;
	}
	else if (srcAspect < dstAspect)
	{
		float cropHeight = static_cast<float>(r.w) / dstAspect;
		src.y += (static_cast<float>(r.h) - cropHeight) * 0.5f;
		src.h = cropHeight;
	}

	SDL_RenderTexture(renderer, texture, &src, dstrect);
}

inline void TextureAtlas::drawTiled(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect, float scale)
{
	if (!dstrect || dstrect->w <= 0.0f || dstrect->h <= 0.0f || scale <= 0.0f)
		return;

	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	const SDL_Rect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	SDL_RenderTextureTiled(renderer, texture, &src, scale, dstrect);
}

inline void TextureAtlas::drawFrame(SDL_Renderer *renderer, const std::string &name, int frameWidth, int frameHeight,
	int frameIndex, const SDL_FRect *dstrect, bool fliphoriz)
{
	if (frameIndex < 0 || frameWidth <= 0 || frameHeight <= 0)
		return;

	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	const SDL_Rect& r = it->second;
	int columns = r.w / frameWidth;
	if (columns <= 0)
		return;

	int frameX = frameIndex % columns;
	int frameY = frameIndex / columns;
	if ((frameY + 1) * frameHeight > r.h)
		return;

	SDL_FRect src = {
		(float)r.x + frameX * frameWidth,
		(float)r.y + frameY * frameHeight,
		(float)frameWidth,
		(float)frameHeight
	};
	SDL_FRect default_dst = {0.0f, 0.0f, (float)frameWidth, (float)frameHeight};
	SDL_RenderTextureRotated(renderer, texture, &src, dstrect ? dstrect : &default_dst, 0.0, nullptr,
		fliphoriz ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

inline void TextureAtlas::drawFrameSection(SDL_Renderer *renderer, const std::string &name, int frameWidth,
	int frameHeight, int frameIndex, const SDL_FRect *dstrect, float cropLeft, float cropTop, float cropRight,
	float cropBottom, bool fliphoriz)
{
	if (frameIndex < 0 || frameWidth <= 0 || frameHeight <= 0)
		return;

	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	float croppedWidth = static_cast<float>(frameWidth) - cropLeft - cropRight;
	float croppedHeight = static_cast<float>(frameHeight) - cropTop - cropBottom;
	if (croppedWidth <= 0.0f || croppedHeight <= 0.0f)
		return;

	const SDL_Rect& r = it->second;
	int columns = r.w / frameWidth;
	if (columns <= 0)
		return;

	int frameX = frameIndex % columns;
	int frameY = frameIndex / columns;
	if ((frameY + 1) * frameHeight > r.h)
		return;

	SDL_FRect src = {
		(float)r.x + frameX * frameWidth + cropLeft,
		(float)r.y + frameY * frameHeight + cropTop,
		croppedWidth,
		croppedHeight
	};

	SDL_FRect defaultDst = {cropLeft, cropTop, croppedWidth, croppedHeight};
	SDL_FRect croppedDst = defaultDst;
	if (dstrect)
	{
		float scaleX = dstrect->w / static_cast<float>(frameWidth);
		float scaleY = dstrect->h / static_cast<float>(frameHeight);
		croppedDst = {
			dstrect->x + cropLeft * scaleX,
			dstrect->y + cropTop * scaleY,
			croppedWidth * scaleX,
			croppedHeight * scaleY
		};
	}

	SDL_RenderTextureRotated(renderer, texture, &src, &croppedDst, 0.0, nullptr,
		fliphoriz ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

inline void TextureAtlas::drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, const SDL_FRect& dstrect)
{
	if (index < 0)
		return;
	
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}
	
	const SDL_Rect& r = it->second;
	int iw = index % (r.w / TILESIZE);
	int ih = index / (r.w / TILESIZE);
	SDL_FRect src = { (float)r.x + iw * TILESIZE, (float)r.y + ih * TILESIZE, (float)TILESIZE, (float)TILESIZE };	
	SDL_RenderTexture(renderer, texture, &src, &dstrect);
}

inline void TextureAtlas::drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, int x, int y)
{
	SDL_FRect dst = {
		static_cast<float>(x * TILESIZE),
		static_cast<float>(y * TILESIZE),
		static_cast<float>(TILESIZE),
		static_cast<float>(TILESIZE)
	};
	drawTile(renderer, name, index, dst);
}
