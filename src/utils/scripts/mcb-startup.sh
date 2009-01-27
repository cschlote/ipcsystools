# !/bin/bash
#
# Startskript f�r die Mobile Connect Box (MCB-2)
#

# Basis Bibliothek f�r die MCB-2
. /usr/share/mcbsystools/mcblib.inc

# Verbindungsstatus f�r das Starten der MCB eintragen
echo `date +%s`@10 > $MCB_STATUSFILE_DIR/connection.state

echo `date` "  ;mcb-startup" $@ >> $CONNECTION_LOG_FILE

# Einwahlskript f�r die UMTS Verbindung starten    
if (test $START_UMTS_ENABLED -eq 1); then 

  # VPN Verbindung wird �ber das UMTS Sktipt gestartet
  /usr/share/mcbsystools/umts-connection.sh start

fi  

     


