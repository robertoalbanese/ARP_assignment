Configuration File:

1 Next student IP
2 Socket port for communication
3 Waiting time (microseconds)
4 RF value 

TO DO:
- IMPLEMENTA CALCOLO NEW TOKEN. (printa i valori della formula in un txt per plottare la sinusoide)
- Scrivere how to per invio di segnali
- Modifica file log: aggiungi il nome del processo oltre al pid (G o S)
- Aggiungi waiting time come parametro d'esecuzione ed aggiungere delay totale nella formula (usa waiting time come delay di comunicazione in ricezione)
    P:  riceve il valore da G
        calcola il nuovo TOKEN
        invia il messaggio con il nuovo token e con il timestamp
        applica sleep(waiting_time)
    Nel giro successivo P terrà conto del delay per il calcolo di DT

- !!!! Nel REPORT:    dire che S è il padre di tutti i processi perchè se fosse un figlio, dopo uno stop, non sarebbe più capace di ricevere 
                    dei segnali di resume.

- Controllare i segnali usati nel sig_handler