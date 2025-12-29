import pygame

class Pause_Menu:
    def __init__(self, Game):
        self.Game = Game
        self.pause_activated = False
        self.trigger = 0
    def check_for_pause(self):
        if self.Game.Player_obj.keys[pygame.K_ESCAPE]:
            self.pause_activated = True
            self.trigger = 5
            self._pause_loop()
    def _pause_loop(self):
        while self.pause_activated:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.Game.running = False
                    self.pause_activated = False
            self.Game.Player_obj.keys = pygame.key.get_pressed()
            if self.Game.Player_obj.keys[pygame.K_ESCAPE] and self.trigger<=0:
                self.pause_activated = False
            self.Game.clock.tick(20)
            self.trigger -= 1