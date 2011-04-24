;# data.s	
;# checks all the data transfer instructions
foo:
	MOVE DPC, #01h
	MOVE DPC, AP
	MOVE DPC, APC
	MOVE DPC, PSF
	MOVE DPC, IC
	MOVE DPC, IMR
	MOVE DPC, SC
	MOVE DPC, IIR
	MOVE DPC, CKCN
	MOVE DPC, WDCN
	MOVE DPC, A[0]		;Just Check two boundary conditions
	MOVE DPC, A[15]   
	MOVE DPC, ACC
	MOVE DPC, A[AP]
	MOVE DPC, IP
	MOVE DPC, @SP--
	MOVE DPC, SP
	MOVE DPC, IV
	MOVE DPC, LC[0]
	MOVE DPC, LC[1]
	MOVE DPC, @BP[OFFS++]
	MOVE DPC, @BP[OFFS--]
	MOVE DPC, OFFS
	MOVE DPC, GR
	MOVE DPC, GRL
	MOVE DPC, BP
	MOVE DPC, GRS
	MOVE DPC, GRH
	MOVE DPC, GRXL
	MOVE DPC, FP
	MOVE DPC, @DP[0]	
	MOVE DPC, @DP[1]	
	MOVE DPC, @DP[0]++		
	MOVE DPC, @DP[1]++
	MOVE DPC, @DP[0]--
	MOVE DPC, @DP[1]--
	MOVE GR, #01h
	MOVE GR, AP
	MOVE GR, APC
	MOVE GR, PSF
	MOVE GR, IC
	MOVE GR, IMR
	MOVE GR, SC
	MOVE GR, IIR
	MOVE GR, CKCN
	MOVE GR, WDCN
	MOVE GR, A[0]		;Just Check two boundary conditions
	MOVE GR, A[15]   
	MOVE GR, ACC
	MOVE GR, A[AP]
	MOVE GR, IP
	MOVE GR, @SP--
	MOVE GR, SP
	MOVE GR, IV
	MOVE GR, LC[0]
	MOVE GR, LC[1]
	MOVE GR, @BP[OFFS++]
	MOVE GR, @BP[OFFS--]
	MOVE GR, OFFS
	MOVE GR, DPC
	MOVE GR, GRL
	MOVE GR, BP
	MOVE GR, GRS
	MOVE GR, GRH
	MOVE GR, GRXL
	MOVE GR, FP
	MOVE GR, @DP[0]	
	MOVE GR, @DP[1]	
	MOVE GR, @DP[0]++		
	MOVE GR, @DP[1]++
	MOVE GR, @DP[0]--
	MOVE GR, @DP[1]--
	MOVE GRL, #01h
	MOVE GRL, AP
	MOVE GRL, APC
	MOVE GRL, PSF
	MOVE GRL, IC
	MOVE GRL, IMR
	MOVE GRL, SC
	MOVE GRL, IIR
	MOVE GRL, CKCN
	MOVE GRL, WDCN
	MOVE GRL, A[0]		;Just Check two boundary conditions
	MOVE GRL, A[15]   
	MOVE GRL, ACC
	MOVE GRL, A[AP]
	MOVE GRL, IP
	MOVE GRL, @SP--
	MOVE GRL, SP
	MOVE GRL, IV
	MOVE GRL, LC[0]
	MOVE GRL, LC[1]
	MOVE GRL, @BP[OFFS++]
	MOVE GRL, @BP[OFFS--]
	MOVE GRL, OFFS
	MOVE GRL, DPC
	MOVE GRL, GR
	MOVE GRL, BP
	MOVE GRL, GRS
	MOVE GRL, GRH
	MOVE GRL, GRXL
	MOVE GRL, FP
	MOVE GRL, @DP[0]	
	MOVE GRL, @DP[1]	
	MOVE GRL, @DP[0]++		
	MOVE GRL, @DP[1]++
	MOVE GRL, @DP[0]--
	MOVE GRL, @DP[1]--
	MOVE BP, #01h
	MOVE BP, AP
	MOVE BP, APC
	MOVE BP, PSF
	MOVE BP, IC
	MOVE BP, IMR
	MOVE BP, SC
	MOVE BP, IIR
	MOVE BP, CKCN
	MOVE BP, WDCN
	MOVE BP, A[0]		;Just Check two boundary conditions
	MOVE BP, A[15]   
	MOVE BP, ACC
	MOVE BP, A[AP]
	MOVE BP, IP
	MOVE BP, @SP--
	MOVE BP, SP
	MOVE BP, IV
	MOVE BP, LC[0]
	MOVE BP, LC[1]
	MOVE BP, @BP[OFFS++]
	MOVE BP, @BP[OFFS--]
	MOVE BP, OFFS
	MOVE BP, DPC
	MOVE BP, GR
	MOVE BP, GRL
	MOVE BP, GRS
	MOVE BP, GRH
	MOVE BP, GRXL
	MOVE BP, FP
	MOVE BP, @DP[0]	
	MOVE BP, @DP[1]	
	MOVE BP, @DP[0]++		
	MOVE BP, @DP[1]++
	MOVE BP, @DP[0]--
	MOVE BP, @DP[1]--
	MOVE @DP[0], #01h
	MOVE @DP[0], AP
	MOVE @DP[0], APC
	MOVE @DP[0], PSF
	MOVE @DP[0], IC
	MOVE @DP[0], IMR
	MOVE @DP[0], SC
	MOVE @DP[0], IIR
	MOVE @DP[0], CKCN
	MOVE @DP[0], WDCN
	MOVE @DP[0], A[0]		;Just Check two boundary conditions
	MOVE @DP[0], A[15]   
	MOVE @DP[0], ACC
	MOVE @DP[0], A[AP]
	MOVE @DP[0], IP
	MOVE @DP[0], @SP--
	MOVE @DP[0], SP
	MOVE @DP[0], IV
	MOVE @DP[0], LC[0]
	MOVE @DP[0], LC[1]
	MOVE @DP[0], @BP[OFFS++]
	MOVE @DP[0], @BP[OFFS--]
	MOVE @DP[0], OFFS
	MOVE @DP[0], DPC
	MOVE @DP[0], GR
	MOVE @DP[0], GRL
	MOVE @DP[0], BP
	MOVE @DP[0], GRS
	MOVE @DP[0], GRH
	MOVE @DP[0], GRXL
	MOVE @DP[0], FP
	MOVE @++DP[0], #01h
	MOVE @++DP[0], AP
	MOVE @++DP[0], APC
	MOVE @++DP[0], PSF
	MOVE @++DP[0], IC
	MOVE @++DP[0], IMR
	MOVE @++DP[0], SC
	MOVE @++DP[0], IIR
	MOVE @++DP[0], CKCN
	MOVE @++DP[0], WDCN
	MOVE @++DP[0], A[0]		;Just Check two boundary conditions
	MOVE @++DP[0], A[15]   
	MOVE @++DP[0], ACC
	MOVE @++DP[0], A[AP]
	MOVE @++DP[0], IP
	MOVE @++DP[0], @SP--
	MOVE @++DP[0], SP
	MOVE @++DP[0], IV
	MOVE @++DP[0], LC[0]
	MOVE @++DP[0], LC[1]
	MOVE @++DP[0], @BP[OFFS++]
	MOVE @++DP[0], @BP[OFFS--]
	MOVE @++DP[0], OFFS
	MOVE @++DP[0], DPC
	MOVE @++DP[0], GR
	MOVE @++DP[0], GRL
	MOVE @++DP[0], BP
	MOVE @++DP[0], GRS
	MOVE @++DP[0], GRH
	MOVE @++DP[0], GRXL
	MOVE @++DP[0], FP
	MOVE @--DP[0], #01h
	MOVE @--DP[0], AP
	MOVE @--DP[0], APC
	MOVE @--DP[0], PSF
	MOVE @--DP[0], IC
	MOVE @--DP[0], IMR
	MOVE @--DP[0], SC
	MOVE @--DP[0], IIR
	MOVE @--DP[0], CKCN
	MOVE @--DP[0], WDCN
	MOVE @--DP[0], A[0]		;Just Check two boundary conditions
	MOVE @--DP[0], A[15]   
	MOVE @--DP[0], ACC
	MOVE @--DP[0], A[AP]
	MOVE @--DP[0], IP
	MOVE @--DP[0], @SP--
	MOVE @--DP[0], SP
	MOVE @--DP[0], IV
	MOVE @--DP[0], LC[0]
	MOVE @--DP[0], LC[1]
	MOVE @--DP[0], @BP[OFFS++]
	MOVE @--DP[0], @BP[OFFS--]
	MOVE @--DP[0], OFFS
	MOVE @--DP[0], DPC
	MOVE @--DP[0], GR
	MOVE @--DP[0], GRL
	MOVE @--DP[0], BP
	MOVE @--DP[0], GRS
	MOVE @--DP[0], GRH
	MOVE @--DP[0], GRXL
	MOVE @--DP[0], FP
	MOVE DP[0], #01h
	MOVE DP[0], AP
	MOVE DP[0], APC
	MOVE DP[0], PSF
	MOVE DP[0], IC
	MOVE DP[0], IMR
	MOVE DP[0], SC
	MOVE DP[0], IIR
	MOVE DP[0], CKCN
	MOVE DP[0], WDCN
	MOVE DP[0], A[0]		;Just Check two boundary conditions
	MOVE DP[0], A[15]   
	MOVE DP[0], ACC
	MOVE DP[0], A[AP]
	MOVE DP[0], IP
	MOVE DP[0], @SP--
	MOVE DP[0], SP
	MOVE DP[0], IV
	MOVE DP[0], LC[0]
	MOVE DP[0], LC[1]
	MOVE DP[0], @BP[OFFS++]
	MOVE DP[0], @BP[OFFS--]
	MOVE DP[0], OFFS
	MOVE DP[0], DPC
	MOVE DP[0], GR
	MOVE DP[0], GRL
	MOVE DP[0], BP
	MOVE DP[0], GRS
	MOVE DP[0], GRH
	MOVE DP[0], GRXL
	MOVE DP[0], FP
	MOVE SC, #01h
	MOVE SC, AP
	MOVE SC, APC
	MOVE SC, PSF
	MOVE SC, IC
	MOVE SC, IMR
	MOVE SC, IIR
	MOVE SC, CKCN
	MOVE SC, WDCN
	MOVE SC, A[0]		;Just Check two boundary conditions
	MOVE SC, A[15]   
	MOVE SC, ACC
	MOVE SC, A[AP]
	MOVE SC, IP
	MOVE SC, @SP--
	MOVE SC, SP
	MOVE SC, IV
	MOVE SC, LC[0]
	MOVE SC, LC[1]
	MOVE SC, @BP[OFFS++]
	MOVE SC, @BP[OFFS--]
	MOVE SC, OFFS
	MOVE SC, DPC
	MOVE SC, GR
	MOVE SC, GRL
	MOVE SC, BP
	MOVE SC, GRS
	MOVE SC, GRH
	MOVE SC, GRXL
	MOVE SC, FP
	MOVE SC, @DP[0]	
	MOVE SC, @DP[1]	
	MOVE SC, @DP[0]++		
	MOVE SC, @DP[1]++
	MOVE SC, @DP[0]--
	MOVE SC, @DP[1]--
	MOVE CKCN, #01h
	MOVE CKCN, AP
	MOVE CKCN, APC
	MOVE CKCN, PSF
	MOVE CKCN, IC
	MOVE CKCN, IMR
	MOVE CKCN, SC
	MOVE CKCN, IIR
	MOVE CKCN, WDCN
	MOVE CKCN, A[0]		;Just Check two boundary conditions
	MOVE CKCN, A[15]   
	MOVE CKCN, ACC
	MOVE CKCN, A[AP]
	MOVE CKCN, IP
	MOVE CKCN, @SP--
	MOVE CKCN, SP
	MOVE CKCN, IV
	MOVE CKCN, LC[0]
	MOVE CKCN, LC[1]
	MOVE CKCN, @BP[OFFS++]
	MOVE CKCN, @BP[OFFS--]
	MOVE CKCN, OFFS
	MOVE CKCN, DPC
	MOVE CKCN, GR
	MOVE CKCN, GRL
	MOVE CKCN, BP
	MOVE CKCN, GRS
	MOVE CKCN, GRH
	MOVE CKCN, GRXL
	MOVE CKCN, FP
	MOVE CKCN, @DP[0]	
	MOVE CKCN, @DP[1]	
	MOVE CKCN, @DP[0]++		
	MOVE CKCN, @DP[1]++
	MOVE CKCN, @DP[0]--
	MOVE CKCN, @DP[1]--
	MOVE GRH, #01h
	MOVE GRH, AP
	MOVE GRH, APC
	MOVE GRH, PSF
	MOVE GRH, IC
	MOVE GRH, IMR
	MOVE GRH, SC
	MOVE GRH, IIR
	MOVE GRH, CKCN
	MOVE GRH, WDCN
	MOVE GRH, A[0]		;Just Check two boundary conditions
	MOVE GRH, A[15]   
	MOVE GRH, ACC
	MOVE GRH, A[AP]
	MOVE GRH, IP
	MOVE GRH, @SP--
	MOVE GRH, SP
	MOVE GRH, IV
	MOVE GRH, LC[0]
	MOVE GRH, LC[1]
	MOVE GRH, @BP[OFFS++]
	MOVE GRH, @BP[OFFS--]
	MOVE GRH, OFFS
	MOVE GRH, DPC
	MOVE GRH, GR
	MOVE GRH, GRL
	MOVE GRH, BP
	MOVE GRH, GRS
	MOVE GRH, GRXL
	MOVE GRH, FP
	MOVE GRH, @DP[0]	
	MOVE GRH, @DP[1]	
	MOVE GRH, @DP[0]++		
	MOVE GRH, @DP[1]++
	MOVE GRH, @DP[0]--
	MOVE GRH, @DP[1]--
