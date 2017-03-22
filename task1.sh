#!/bin/bash


#     1 | 2 | 3
#     4 | 5 | 6
#     7 | 8 | 9

line="  "---------"|"---------"|"---------" "
positions=(- - - - - - - - -)
player_one=false
game_finished=false
repeat=false

if ! [ -p pipe1 ]; then
	mkfifo pipe1
	player_one=true
	sign="X"
else
	sign="O"
fi

#рисует поле ¯\_(ツ)_/¯
function draw_board {
  clear
  name=$1[@]
  positions=("${!name}")

  for (( row_id=1; row_id<=3; row_id++ ));do
    row="  "
    empty_row="  "
    for (( col_id=1; col_id<$((10*3)); col_id++ ));do
      if [[ $(( $col_id%10 )) == 0 ]]; then
        row=$row"|"
        empty_row=$empty_row"|"
      else
        if [[ $(( $col_id%5 )) == 0 ]]; then
          x=$(($row_id-1))
          y=$((($col_id - 5) / 10))

          if [[ $x == 0 ]]; then
            what=${positions[$y]}
          elif [[ $x == 1 ]]; then
            what=${positions[(($y+3))]}
          else
            what=${positions[(($y+6))]}
          fi

          if [[ $what == "-" ]]; then what=" "; fi

          if [[ $what == "X" ]] ; then
            row=$row$what
          else
            row=$row$what
          fi

          empty_row=$empty_row" "
        else
          row=$row" "
          empty_row=$empty_row" "
        fi
      fi
    done
    echo -e "$empty_row""\n""$row""\n""$empty_row"
    if [[ $row_id != 3 ]]; then
      echo -e "$line"
    fi
  done
  echo -e "\n"
}

function end_game {
  game_finished=true
  draw_board positions
  if [ -p pipe1 ]; then
    rm pipe1
  fi
}

#проверка конца игры
function test_position_str {
  rows=${1:0:3}" "${1:3:3}" "${1:6:8}
  cols=${1:0:1}${1:3:1}${1:6:1}" "${1:1:1}${1:4:1}${1:7:1}" "${1:2:1}${1:5:1}${1:8:1}
  diagonals=${1:0:1}${1:4:1}${1:8:1}" "${1:2:1}${1:4:1}${1:6:1}
  if [[ $rows =~ [X]{3,} || $cols =~ [X]{3,} || $diagonals =~ [X]{3,} ]]; then
    end_game
    if [ "$player_one" = true ] ; then
	echo "You won!"
    else
	echo "You lose!"
    fi
    return
  fi
  if [[ $rows =~ [O]{3,} || $cols =~ [O]{3,} || $diagonals =~ [O]{3,} ]]; then
    end_game
    if [ "$player_one" = true ] ; then
	echo "You won!"
    else
	echo "You lose!"
    fi
    return
  fi
  if [[ ! $positions_str =~ [-] ]]; then
    end_game
    echo "Draw!"
  fi
}

#основной цикл игры
while true
do
  draw_board positions
  if [ "$repeat" = false ] ; then
    if [ "$player_one" = true ] ; then
      echo "Your move"
    else
      echo "Waiting..."

      #читаем данные
      msg=$(cat < pipe1)
      index=${msg:0:1}
      sign2=${msg:2:3}

      positions["$index"]=$sign2
      draw_board positions

      positions_str=$(printf "%s" "${positions[@]}")
      test_position_str $positions_str

      if [ "$game_finished" = false ] ; then
	echo "Your move"
        player_one=true
      else
	break
      fi
    fi
  else
    echo -e "Can't move here\nYour move"
    repeat=false
  fi

  read -d'' -s -n1 input
  case $input in
        1) index=0;;
        2) index=1;;
        3) index=2;;
        4) index=3;;
        5) index=4;;
        6) index=5;;
        7) index=6;;
        8) index=7;;
        9) index=8;;
  esac

  if [ "${positions["$index"]}" == "-" ]; then
    positions["$index"]=$sign
    #отправляем данные
    echo "$index,$sign" > pipe1
  else
    repeat=true
  fi

  positions_str=$(printf "%s" "${positions[@]}")
  test_position_str $positions_str
  if [ "$game_finished" = false ] ; then
    player_one=false
  else
    player_one=true
    break
  fi
done