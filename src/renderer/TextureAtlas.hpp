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
	inline void draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale = 1.0f, bool fliphoriz = false);
	inline void draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect);
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

inline void TextureAtlas::draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale, bool fliphoriz)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}
	int flip = 1;
	if (fliphoriz)
	{
		flip = -1;
	}

	const SDL_Rect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	SDL_FRect dst = {(float)x, (float)y, (float)r.w * scale * flip, (float)r.h * scale};
	SDL_RenderTexture(renderer, texture, &src, &dst);
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

inline void TextureAtlas::drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, int x, int y)
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
	SDL_FRect dst = { (float)x * 3 * TILESIZE, (float)y * 3 * TILESIZE, (float)TILESIZE * 3, (float)TILESIZE * 3};	
	SDL_RenderTexture(renderer, texture, &src, &dst);
}
