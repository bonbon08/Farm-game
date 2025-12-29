import random
import pygame

class Player_class:
    def __init__(self, Game):
        self.Game = Game
        self.x = 0
        self.y = 0
        self.facing_position = 0
        self.trigger = 0
    def move(self):
        self.keys = pygame.key.get_pressed()
        if self.keys[pygame.K_w]:
            if self.y > 0:
                self.y -= self.Game.speed
            self.facing_position = 2
        if self.keys[pygame.K_s]:
            if self.y < 464:
                self.y += self.Game.speed
            self.facing_position = 0
        if self.keys[pygame.K_a]:
            if self.x>0:
                self.x -= self.Game.speed
            self.facing_position = 1
        if self.keys[pygame.K_d]:
            if self.x < 624:
                self.x += self.Game.speed
            self.facing_position = 3
        if self.keys[pygame.K_x] and self.trigger <= 0:
            print(self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0])
            match self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0]:
                case 0:
                    if self.Game.money >= 1:
                        self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0] = 1
                        self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][1] = random.randint(100,10000)
                        self.Game.money -= 1
                case 1:
                    self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0] = 0
                case 2:
                    self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0] = 0
                    self.Game.money += 10
                case 4:
                    self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0] = 0
                    self.Game.money -= random.randint(1,30)
                case 3:
                    self.Game.wmap.matrix[self.Game.wmap.return_tile_x][self.Game.wmap.return_tile_y][0] = 0
            self.trigger = 20
        self.trigger -= 1