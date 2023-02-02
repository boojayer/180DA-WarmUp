import random
import time

print('Starting Rock Paper Scissors...')
winState = 'You won!'
loseState = 'You lost!'
choices = ['r', 'p', 's']

for i in range(3):
  user_input = input("(type r, p, or s): ")
  time.sleep(1)
  if user_input not in choices:
    print('invalid input')
    continue

  computerChoice = choices[random.randint(0,2)]
  print('The computer chose ' + computerChoice)

  if computerChoice is user_input:
    print('Tie!')
  elif user_input == 'r':
    print(winState if computerChoice == 's' else loseState)
  elif user_input == 'p':
    print(winState if computerChoice == 'r' else loseState)
  elif user_input == 's':
    print(winState if computerChoice == 'p' else loseState)