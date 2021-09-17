Configuration File:

1 Next student IP
2 Socket port for communication
3 Waiting time (microseconds)
4 RF value 

TO DO:
- implementare: S manda tipo segnale a P tramite pipe
	- P se dump -> usa la pipe per loggare tramite L
	- (opzionale) segnale blocca e riattiva P tramite flag
- Scrivere how to per invio di segnali
- Aggiustare Log file deve gestire i dati di tipo struct msg e non float
- Modifica file log: aggiungi il nome del processo oltre al pid (G o S)


- !!!! Nel REPORT:    dire che S è il padre di tutti i processi perchè se fosse un figlio, dopo uno stop, non sarebbe più capace di ricevere 
                    dei segnali di resume. Se entrambe le pipe sono pine do la priorità ad S

- Controllare i segnali usati nel sig_handler
