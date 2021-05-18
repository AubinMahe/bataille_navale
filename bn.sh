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

if [ $# -eq 0 ] ; then
   usage
fi

if [ "$2" == "" ] ; then
   gnome-terminal --geometry 84x25+0-0 -- $1 --nom=Aubin --journal=/tmp/bataille_navale &
else
   case "$2" in
   ia)
      gnome-terminal --geometry 84x25+1000+500 -- $1 --nom=Aubin --journal=/tmp/bataille_navale &
   ;;
   test)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=127.0.0.1:2416/127.0.0.1:2417 --journal=/tmp/bataille_navale &
      gnome-terminal --geometry 84x25+0-0 -- $1 --nom=Muriel --reseau=127.0.0.1:2417/127.0.0.1:2416 --journal=/tmp/bataille_navale &
   ;;
   test-wine)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=127.0.0.1:2416/127.0.0.1:2417 --journal=/tmp/bataille_navale &
      export TERM=xterm
      gnome-terminal --geometry 84x25+0-0 -- wine TERM=xterm BUILD/bataille_navale.exe --nom=Muriel --reseau=127.0.0.1:2417/127.0.0.1:2416 --journal=/tmp/bataille_navale &
   ;;
   externe)
      gnome-terminal --geometry 84x25-0-0 -- $1 --nom=Aubin  --reseau=192.168.1.10:2416/10.0.2.15:2417 --journal=/tmp/bataille_navale &
   ;;
   *)
      usage
   ;;
   esac
fi
