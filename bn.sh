#!/bin/bash

function usage() {
   echo "Arguments : l'exécutable à lancer : BUILD/bataille_navale (par défaut) ou Debug/bataille_navale,"
   echo "            éventuellement suivi de 'ia', 'test', 'test-wine' ou 'externe'"
   echo "   • 'ia' (par défaut) : pour jouer contre l'ordinateur"
   echo "   • 'test'            : lance deux instances côte-à-côte"
   echo "   • 'test-wine'       : lance deux instances côte-à-côte, la seconde sous wine"
   echo "   • 'externe'         : lance une seule instance, sur une IP externe"
   exit 1
}

JOURNAL=--journal=/tmp/bataille_navale
LD_LIBRARY_PATH=$(realpath ./BUILD)

if [ $# -eq 0 ] ; then
   gnome-terminal --geometry 84x25+600+250 -- BUILD/bataille_navale --nom=Aubin $JOURNAL &
elif [ "$2" == "" ] ; then
   gnome-terminal --geometry 84x25+600+250 -- $1 --nom=Aubin $JOURNAL &
else
   case "$2" in
   ia)
      gnome-terminal --geometry 84x25+600+250 -- $1 --nom=Aubin $JOURNAL &
   ;;
   test)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=127.0.0.1:2416/127.0.0.1:2417 $JOURNAL &
      gnome-terminal --geometry 84x25+0-0 -- $1 --nom=Muriel --reseau=127.0.0.1:2417/127.0.0.1:2416 $JOURNAL &
   ;;
   test-wine)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=127.0.0.1:2416/127.0.0.1:2417 $JOURNAL &
      export TERM=xterm
      cd BUILD
      #gnome-terminal --geometry 84x25+0-0 -- wine bataille_navale --nom=Muriel --reseau=127.0.0.1:2417/127.0.0.1:2416 $JOURNAL &
      wineconsole
      # bataille_navale --nom=Muriel --reseau=127.0.0.1:2417/127.0.0.1:2416 --journal=/tmp/bataille_navale
   ;;
   externe)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=192.168.1.10:2416/10.0.2.15:2417 $JOURNAL &
   ;;
   *)
      usage
   ;;
   esac
fi
