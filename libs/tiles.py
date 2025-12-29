import pygame

pygame.init()

class TileSet:
    def __init__(self, image_path, tile_width, tile_height):
        self.image = pygame.image.load(image_path).convert_alpha()
        self.tile_width = tile_width
        self.tile_height = tile_height

        self.tiles = self._slice_tiles()

    def _slice_tiles(self):
        tiles = []
        image_width, image_height = self.image.get_size()
        for y in range(0, image_height, self.tile_height):
            for x in range(0, image_width, self.tile_width):
                rect = pygame.Rect(x, y, self.tile_width, self.tile_height)
                tile = self.image.subsurface(rect)
                tiles.append(tile)
        return tiles

    def get_tile(self, index):
        if index < 0 or index >= len(self.tiles):
            raise IndexError("Tile index out of range.")
        return self.tiles[index]

    def get_tile_xy(self, x, y):
        tiles_per_row = self.image.get_width() // self.tile_width
        index = y * tiles_per_row + x
        return self.get_tile(index)

    def get_tile_scalled(self, index, scale):
        if index < 0 or index >= len(self.tiles):
            raise IndexError("Tile index out of range.")
        return pygame.transform.scale(self.tiles[index], scale)