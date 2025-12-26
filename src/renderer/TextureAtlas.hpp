#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "../constants.hpp"



class TextureAtlas
{
public:
	inline bool load(SDL_Renderer *renderer, const std::string &imagePath, const std::string &xmlPath);
	inline void unload();
	inline void draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale = 1.0f, bool fliphoriz = false);
	inline void draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect);
	inline void drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, float x, float y);

private:
	SDL_Texture *texture = nullptr;
	std::unordered_map<std::string, SDL_Rect> atlas;
};

// Very minimal XML tag parser (enough for <SubTexture name="..." x="..." .../>)
static std::unordered_map<std::string, SDL_Rect> parseAtlasXML(const std::string &xmlPath)
{
	std::unordered_map<std::string, SDL_Rect> atlas;
	std::ifstream file(xmlPath);
	if (!file.is_open())
	{
		std::cerr << "Failed to open atlas XML: " << xmlPath << "\n";
		return atlas;
	}

	std::string line;
	while (std::getline(file, line))
	{
		// remove leading/trailing spaces
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		// skip irrelevant lines
		if (line.empty() || line.find("<SubTexture") == std::string::npos)
			continue;

		std::string name;
		int x = 0, y = 0, w = 0, h = 0;

		auto getAttr = [&](const std::string &key) -> std::string
		{
			size_t pos = line.find(key + "=\"");
			if (pos == std::string::npos)
				return "";
			pos += key.size() + 2;
			size_t end = line.find("\"", pos);
			return line.substr(pos, end - pos);
		};

		name = getAttr("name");
		x = std::stoi(getAttr("x"));
		y = std::stoi(getAttr("y"));
		w = std::stoi(getAttr("width"));
		h = std::stoi(getAttr("height"));

		atlas[name] = {x, y, w, h};
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

inline void TextureAtlas::drawTile(SDL_Renderer* renderer, const std::string& name, int8_t index, float x, float y)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}

	if (index < 0)
		index = 0;

	const SDL_Rect& r = it->second;
	int iw = index % (r.w / TILESIZE);
	int ih = index / (r.w / TILESIZE);
	SDL_FRect src = { (float)r.x + iw * TILESIZE, (float)r.y + ih * TILESIZE, (float)TILESIZE, (float)TILESIZE };
	SDL_FRect dst = { (float)x, (float)y, (float)TILESIZE, (float)TILESIZE};
	SDL_RenderTexture(renderer, texture, &src, &dst);
}
