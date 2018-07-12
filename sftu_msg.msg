.TITLE	SFTU Server message file
.IDENT	'VERSION 1.0'

!++
!-----------------------------------------------------------------------------------
!	Server specific message section
!-----------------------------------------------------------------------------------
!--
.facility	SFTU,53  /prefix=SFTU_

.severity	informational
!$FIRST		<First MSG marker.>

ACCEPT		<Accept connection request from !AZ:!UW on I/O channel = !UL>/FAO=3

LISTEN		<Start file access listener on channel = !XW (TCP port !UW)>/FAO=2
!.explanation	 None.
!.user_action	 None.

RCVPUT		<Received put request for !AF, EBK = !UL, FFB = !UW>/FAO=4
!.explanation	 None.
!.user_action	 None.

SNDPUT		<Sent request for !AS, EBK = !UL, FFB = !UW, !AZACK>/FAO=5
!.explanation	 None.
!.user_action	 None.


RCVACK		<Received acknownledment, status = %x!XL, VBN = !UL (%x!XL)>/FAO=3
!.explanation	 None.
!.user_action	 None.


SENDING		<Sending !AS from VBN=!UL, !AZACK ...>/FAO=3
!.explanation	 None.
!.user_action	 None.



SENT		<!!AS has been sent, status = %x!XL>/FAO=2
!.explanation	 None.
!.user_action	 None.


ALREADY		<Remote site reject file>
!.explanation	 None.
!.user_action	 None.





$LAST		<Last MSG marker.>

!.LITERAL	NUMSG=(SFTU_$LAST@--3)  !--(SFTU_$FIRST@-3)

.end
