#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iostream>

struct SpriteRect
{
	int x, y, w, h;
};

class TextureAtlas
{
public:
	bool load(SDL_Renderer *renderer, const std::string &imagePath, const std::string &xmlPath);
	void unload();
	void draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale = 1.0f);
	void draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect);
	void drawAll(SDL_Renderer *renderer);

private:
	SDL_Texture *texture = nullptr;
	std::unordered_map<std::string, SpriteRect> atlas;
};

// Very minimal XML tag parser (enough for <SubTexture name="..." x="..." .../>)
static std::unordered_map<std::string, SpriteRect> parseAtlasXML(const std::string &xmlPath)
{
	std::unordered_map<std::string, SpriteRect> atlas;
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

bool TextureAtlas::load(SDL_Renderer *renderer, const std::string &imagePath, const std::string &xmlPath)
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

void TextureAtlas::unload()
{
	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
	atlas.clear();
}

void TextureAtlas::draw(SDL_Renderer *renderer, const std::string &name, float x, float y, float scale)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}
	const SpriteRect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};
	SDL_FRect dst = {(float)x, (float)y, (float)r.w * scale, (float)r.h * scale};
	SDL_RenderTexture(renderer, texture, &src, &dst);
}

void TextureAtlas::draw(SDL_Renderer *renderer, const std::string &name, const SDL_FRect *dstrect)
{
	auto it = atlas.find(name);
	if (it == atlas.end())
	{
		std::cerr << "Sprite not found in atlas: " << name << "\n";
		return;
	}
	const SpriteRect &r = it->second;
	SDL_FRect src = {(float)r.x, (float)r.y, (float)r.w, (float)r.h};

	// If dstrect is null, draw the sprite at its original size at (0,0)
	SDL_FRect default_dst = {0.0f, 0.0f, (float)r.w, (float)r.h};
	SDL_RenderTexture(renderer, texture, &src, dstrect ? dstrect : &default_dst);
}

void TextureAtlas::drawAll(SDL_Renderer *renderer)
{
	for (auto [name, rect] : atlas)
	{
	}
}
