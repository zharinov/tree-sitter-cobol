      *> COBOL Sample Program
       IDENTIFICATION DIVISION.
       PROGRAM-ID. TEST-PROGRAM.

       DATA DIVISION.
       WORKING-STORAGE SECTION.
       01 WS-COUNTER          PIC 9(3)    VALUE 0.
       01 WS-NAME             PIC X(30)   VALUE SPACES.
       01 WS-FLAG             PIC 9       VALUE 0.
           88 WS-FLAG-TRUE    VALUE 1.
           88 WS-FLAG-FALSE   VALUE 0.

       PROCEDURE DIVISION.
       MAIN-PARAGRAPH.
           MOVE "Hello" TO WS-NAME
           DISPLAY WS-NAME
           PERFORM VARYING WS-COUNTER FROM 1 BY 1
                   UNTIL WS-COUNTER > 10
               ADD 1 TO WS-COUNTER
           END-PERFORM
           IF WS-FLAG = 0
               DISPLAY "Flag is zero"
           END-IF
           EVALUATE TRUE
               WHEN WS-COUNTER = ZERO
                   DISPLAY "Counter is zero"
               WHEN OTHER
                   DISPLAY "Counter is not zero"
           END-EVALUATE
           STOP RUN.
