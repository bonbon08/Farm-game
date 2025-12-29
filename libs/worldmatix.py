import random

class Farmmatrix:
    def __init__(self, Game):
        self.Game = Game
        self.matrix = []
        self.max_x = int(Game.displayx/Game.field_size_x)
        self.max_y = int(Game.displayy/Game.field_size_y)
        self.matrix_calc_x = Game.displayx/self.max_x
        self.matrix_calc_y = Game.displayy/self.max_y
        for _ in range(int(self.max_x)):
            subline = []
            for _ in range(int(self.max_y)):
                subline.append([0,0])
            self.matrix.append(subline)
    def check_for_nearest_tile(self):
        return_tile_y = 0
        base_tile_x = int(self.Game.Player_obj.x/self.matrix_calc_x)
        base_tile_y = int(self.Game.Player_obj.y/self.matrix_calc_y)
        self.return_tile_x = base_tile_x
        self.return_tile_y = base_tile_y
        match self.Game.Player_obj.facing_position:
            case 0:
                if base_tile_y < self.max_y-1:
                    self.return_tile_y = base_tile_y + 1
            case 1:
                if base_tile_x>0:
                    self.return_tile_x = base_tile_x - 1
            case 2:
                if base_tile_y>0:
                    self.return_tile_y = base_tile_y - 1
            case 3:
                if base_tile_x<self.max_x-1:
                    self.return_tile_x = base_tile_x + 1
    def update(self): 
        xs = 0
        ys = 0
        for x in self.matrix:
            for y in x:
                if y[0] == 1 and random.randint(0,1000000)==0:
                    self.matrix[xs][ys][0] = 4
                if y[0] in [1,2] and y[1]<=0:
                    self.matrix[xs][ys][0] += 1
                    self.matrix[xs][ys][1] = random.randint(1000,10000)
                self.matrix[xs][ys][1] -= 1
                ys += 1
            xs += 1
            ys = 0