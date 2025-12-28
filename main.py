import pygame

from libs import player, worldmatix


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
    def tick(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
        # Game Logik
        self.Player_obj.move()
        self.wmap.check_for_nearest_tile()
        self.wmap.update()
        text = self.gamefont.render(str(self.money), False, (0, 0, 0))
        # Draw mechanic
        self.screen.fill("blue")
        xs = 0
        ys = 0
        color = "green"
        for x in self.wmap.matrix:
            for y in x:
                match y[0]:
                    case 0:
                        color = "green"
                    case 1:
                        color = "yellow"
                    case 2:
                        color = "orange"
                    case 3:
                        color = "red"
                    case 4:
                        color = "black"
                pygame.draw.rect(self.screen, color, (xs*self.wmap.matrix_calc_x, ys*self.wmap.matrix_calc_y, self.field_size_x, self.field_size_y))
                ys += 1
            #print(self.wmap.return_tile_x, self.wmap.return_tile_y, self.money, self.wmap.matrix)
            xs += 1
            ys = 0
        self.Player_obj.move()
        pygame.draw.rect(self.screen, "grey", (self.wmap.return_tile_x*self.wmap.matrix_calc_x, self.wmap.return_tile_y*self.wmap.matrix_calc_y, self.field_size_x, self.field_size_y))
        pygame.draw.rect(self.screen, "red", (self.Player_obj.x, self.Player_obj.y, 8, 8))
        self.screen.blit(text, (4,400))
        pygame.display.flip()
        # Debug
    def mainloop(self):
        while self.running:
            self.tick()
            self.clock.tick(60)
    pygame.quit()
Game().mainloop()