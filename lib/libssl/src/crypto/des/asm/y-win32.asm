	; Don't even think of reading this code
	; It was automatically generated by crypt586.pl
	; Which is a perl program used to generate the x86 assember for
	; any of elf, a.out, BSDI,Win32, or Solaris
	; eric <eay@cryptsoft.com>
	; 
	TITLE	crypt586.asm
        .386
.model FLAT
_TEXT	SEGMENT
PUBLIC	_fcrypt_body
EXTRN   _des_SPtrans:DWORD
_fcrypt_body PROC NEAR
	push	ebp
	push	ebx
	push	esi
	push	edi
	; 
	; Load the 2 words
	xor	edi,		edi
	xor	esi,		esi
	mov	ebp,		DWORD PTR 24[esp]
	push	25
L000start:
	; 
	; Round 0
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR [ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 4[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 1
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 8[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 12[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 2
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 16[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 20[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 3
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 24[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 28[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 4
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 32[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 36[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 5
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 40[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 44[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 6
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 48[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 52[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 7
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 56[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 60[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 8
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 64[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 68[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 9
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 72[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 76[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 10
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 80[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 84[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 11
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 88[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 92[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 12
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 96[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 100[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 13
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 104[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 108[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	; 
	; Round 14
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		esi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		esi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 112[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 116[ebp]
	xor	eax,		esi
	xor	edx,		esi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	edi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	edi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	edi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	edi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	edi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	edi,		ebx
	; 
	; Round 15
	mov	eax,		DWORD PTR 32[esp]
	mov	edx,		edi
	shr	edx,		16
	mov	ecx,		DWORD PTR 36[esp]
	xor	edx,		edi
	and	eax,		edx
	and	edx,		ecx
	mov	ebx,		eax
	shl	ebx,		16
	mov	ecx,		edx
	shl	ecx,		16
	xor	eax,		ebx
	xor	edx,		ecx
	mov	ebx,		DWORD PTR 120[ebp]
	xor	eax,		ebx
	mov	ecx,		DWORD PTR 124[ebp]
	xor	eax,		edi
	xor	edx,		edi
	xor	edx,		ecx
	and	eax,		0fcfcfcfch
	xor	ebx,		ebx
	and	edx,		0cfcfcfcfh
	xor	ecx,		ecx
	mov	bl,		al
	mov	cl,		ah
	ror	edx,		4
	mov	ebp,		DWORD PTR _des_SPtrans[ebx]
	mov	bl,		dl
	xor	esi,		ebp
	mov	ebp,		DWORD PTR _des_SPtrans[0200h+ecx]
	xor	esi,		ebp
	mov	cl,		dh
	shr	eax,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0100h+ebx]
	xor	esi,		ebp
	mov	bl,		ah
	shr	edx,		16
	mov	ebp,		DWORD PTR _des_SPtrans[0300h+ecx]
	xor	esi,		ebp
	mov	ebp,		DWORD PTR 28[esp]
	mov	cl,		dh
	and	eax,		0ffh
	and	edx,		0ffh
	mov	ebx,		DWORD PTR _des_SPtrans[0600h+ebx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0700h+ecx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0400h+eax]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR _des_SPtrans[0500h+edx]
	xor	esi,		ebx
	mov	ebx,		DWORD PTR [esp]
	mov	eax,		edi
	dec	ebx
	mov	edi,		esi
	mov	esi,		eax
	mov	DWORD PTR [esp],ebx
	jnz	L000start
	; 
	; FP
	mov	edx,		DWORD PTR 24[esp]
	ror	edi,		1
	mov	eax,		esi
	xor	esi,		edi
	and	esi,		0aaaaaaaah
	xor	eax,		esi
	xor	edi,		esi
	; 
	rol	eax,		23
	mov	esi,		eax
	xor	eax,		edi
	and	eax,		003fc03fch
	xor	esi,		eax
	xor	edi,		eax
	; 
	rol	esi,		10
	mov	eax,		esi
	xor	esi,		edi
	and	esi,		033333333h
	xor	eax,		esi
	xor	edi,		esi
	; 
	rol	edi,		18
	mov	esi,		edi
	xor	edi,		eax
	and	edi,		0fff0000fh
	xor	esi,		edi
	xor	eax,		edi
	; 
	rol	esi,		12
	mov	edi,		esi
	xor	esi,		eax
	and	esi,		0f0f0f0f0h
	xor	edi,		esi
	xor	eax,		esi
	; 
	ror	eax,		4
	mov	DWORD PTR [edx],eax
	mov	DWORD PTR 4[edx],edi
	pop	ecx
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
_fcrypt_body ENDP
_TEXT	ENDS
END
