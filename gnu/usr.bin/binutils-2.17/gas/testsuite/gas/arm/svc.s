	.text
	.arch armv4t
	.syntax unified
foo:
	swi 0x123456
	swi 0x876543
	svc 0x123456
	svc 0x876543

	.thumb
bar:
	swi 0x5a
	swi 0xa5
	svc 0x5a
	svc 0xa5
