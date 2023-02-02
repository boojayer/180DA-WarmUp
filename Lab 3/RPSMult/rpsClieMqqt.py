import random
import time
import connection


uid = str(random.randint(0, 100000))
client = connection.gen_client()
def on_msg(client, userdata, message):
  global opponentChoice, uid
  data = message.payload.decode('utf-8').split(',')
  if data[0] == uid:
    return
  opponentChoice = data[1]
  # print('Received message: "' + str(message.payload))

client.on_message = on_msg
client.connect_async('mqtt.eclipseprojects.io')
client.loop_start()

print('Starting Rock Paper Scissors...')
winState = 'You won!'
loseState = 'You lost!'
opponentChoice = 'none'
choices = ['r', 'p', 's']
round_num = 0


while True:
  user_input = input("(type r, p, or s): ")
  if user_input not in choices:
    print('invalid input')
    continue

  print('Publishing...', user_input)
  round_num += 1
  client.publish("ece180d/test/A412", uid +','+ user_input +','+ str(round_num), qos=1)
  while opponentChoice not in choices:
    time.sleep(1)


  # opponentChoice = choices[random.randint(0,2)]
  print('The opponent chose ' + opponentChoice)

  if opponentChoice is user_input:
    print('Tie!')
  elif user_input == 'r':
    print(winState if opponentChoice == 's' else loseState)
  elif user_input == 'p':
    print(winState if opponentChoice == 'r' else loseState)
  elif user_input == 's':
    print(winState if opponentChoice == 'p' else loseState)

  opponentChoice = 'none'