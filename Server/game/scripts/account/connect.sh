#!/bin/bash
case "$@" in
   connect) # connect screen
      cat ./txt/acct_connect.txt
      ;;
   account) # account screen
      cat ./txt/acct_account.txt
      ;;
   *) # by default do connect.txt
      cat ./txt/acct_connect.txt
      ;;
esac
