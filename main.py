import pygame

from libs import player, worldmatix, tiles, pause_menu


class Game:
    def __init__(self):
        #Configs
        self.displayx = 640
        self.displayy = 480
        self.matrix_num_x = 16
        self.matrix_num_y = 16
        self.field_size_x = 32
        self.field_size_y = 32
        self.speed = 4
        #Setup
        pygame.init()
        pygame.font.init()
        self.money = 100
        self.gamefont = pygame.font.SysFont("Comic Sans MS", 32)
        self.wmap = worldmatix.Farmmatrix(self)
        self.screen = pygame.display.set_mode((640,480))
        self.clock = pygame.time.Clock()
        self.running = True
        self.Player_obj = player.Player_class(self)
        self.spritesheet = tiles.TileSet("data/SpriteSheet.png", 16, 16)
        self.Pause_Menu = pause_menu.Pause_Menu(self)
        pygame.display.set_caption("Farmgame")
        pygame.display.set_icon(pygame.transform.scale(tiles.TileSet.get_tile(self.spritesheet, 1),(256,256)))
        #self.grass = 
    def tick(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
        # Game Logik
        self.Player_obj.move()
        self.wmap.check_for_nearest_tile()
        self.wmap.update()
        text = self.gamefont.render(str(self.money), False, (0, 0, 0))
        self.Pause_Menu.check_for_pause()
        # Draw mechanic
        self.screen.fill("blue")
        xs = 0
        ys = 0
        color = "green"
        for x in self.wmap.matrix:
            for y in x:
                self.screen.blit(self.spritesheet.get_tile_scalled(4, (32, 32)), (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y))
                match y[0]:
                    case 1:
                        self.screen.blit(self.spritesheet.get_tile_scalled(0, (32, 32)), (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y))
                    case 2:
                        self.screen.blit(self.spritesheet.get_tile_scalled(1, (32, 32)), (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y))
                    case 3:
                        self.screen.blit(self.spritesheet.get_tile_scalled(2, (32, 32)), (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y))
                    case 4:
                        self.screen.blit(self.spritesheet.get_tile_scalled(3, (32, 32)), (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y))
                ys += 1
            xs += 1
            ys = 0
        self.Player_obj.move()
        self.screen.blit(self.spritesheet.get_tile_scalled(11, (32, 32)), (self.wmap.return_tile_x*self.wmap.matrix_calc_x, self.wmap.return_tile_y*self.wmap.matrix_calc_y))
        self.screen.blit(self.spritesheet.get_tile_scalled(5, (32, 32)), (self.Player_obj.x, self.Player_obj.y))
        self.screen.blit(text, (4,400))
        pygame.display.flip()
    def mainloop(self):
        while self.running:
            self.tick()
            self.clock.tick(60)
    pygame.quit()
Game().mainloop()

