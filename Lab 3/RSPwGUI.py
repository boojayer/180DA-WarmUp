import pygame
import random
import time
from pygame.locals import (
    K_r,
    K_p,
    K_s,
    QUIT,
    KEYDOWN,
    K_ESCAPE,
    K_y
)



# Initialize pygame
pygame.init()

# Define constants for the screen width and height
SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600

# Create the screen object
# The size is determined by the constant SCREEN_WIDTH and SCREEN_HEIGHT
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))

# Variable to keep the main loop running
running = True


# Pygame caption
pygame.font.init()
# font
my_font = pygame.font.Font('freesansbold.ttf', 32)

# key presses for game
def update(self, pressed_keys):
    if pressed_keys[K_s]:
        if computer_action == "scissors":
            text_surface3 = my_font.render('Rock smashes scissors! You win.', False, (0, 0, 0))
        else:
            text_surface3 = my_font.render('Paper covers rock! You lose.', False, (0, 0, 0))
        screen.blit(text_surface3, (200, 300))
    if pressed_keys[K_r]:
        if computer_action == "rock":
            print("Paper covers rock! You win!")
        else:
            print("Scissors cuts paper! You lose.")
    if pressed_keys[K_p]:
        if computer_action == "paper":
            print("Scissors cuts paper! You win!")
        else:
            print("Rock smashes scissors! You lose.")


#start variable
start = 1
play = 1

# Main loop
while running:
    # computer actions
    possible_actions = ["rock", "paper", "scissors"]
    computer_action = random.choice(possible_actions)
    if start == 1:
        # fill screen with white
        screen.fill((255, 255, 255))
        # Actual text
        text_surface = my_font.render('Rock Paper Scisscors', False, (0, 0, 0))
        text_surface2 = my_font.render('Press key: "R"-rock, "P"-Paper, "S"-Scissors', False, (0, 0, 0))
        # New surface for text
        screen.blit(text_surface, (200, 300))
        screen.blit(text_surface2, (100, 400))
        start = 0

    # Look at every event in the queue
    for event in pygame.event.get():
        # Did the user click the window close button? If so, stop the loop.
        if event.type == KEYDOWN:
            # Was it the Escape key? If so, stop the loop.
            if event.key == K_ESCAPE:
                running = False
        elif event.type == QUIT:
            running = False

    # Get all the keys currently pressed
    pressed_keys = pygame.key.get_pressed()
    if pressed_keys[K_r]:
        if computer_action == "scissors":
            text_surface3 = my_font.render('Rock smashes scissors! You win.', False, (0, 0, 0))
        else:
            text_surface3 = my_font.render('Paper covers rock! You lose.', False, (0, 0, 0))
        play = 0
        time.sleep(.2)
    elif pressed_keys[K_p]:
        if computer_action == "rock":
            text_surface3 = my_font.render('Paper covers rock! You win!', False, (0, 0, 0))
        else:
            text_surface3 = my_font.render('Scissors cuts paper! You lose.', False, (0, 0, 0))
        play = 0
        time.sleep(.2)
    elif pressed_keys[K_s]:
        if computer_action == "paper":
            text_surface3 = my_font.render('Scissors cuts paper! You win!.', False, (0, 0, 0))
        else:
            text_surface3 = my_font.render('Rock smashes scissors! You lose.', False, (0, 0, 0))
        play = 0
        time.sleep(.2)
    elif pressed_keys[K_y]:
        start = 1
        play = 1

     # publish results
    if play == 0:
        screen.fill((255, 255, 255))
        screen.blit(text_surface3, (200, 300))
        text_surface4 = my_font.render('Play Again? Press "y"', False, (0, 0, 0))
        screen.blit(text_surface4, (200, 400))


    pygame.display.update()