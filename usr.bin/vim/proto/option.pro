/*	$OpenBSD: option.pro,v 1.2 1996/09/21 06:23:53 downsj Exp $	*/
/* option.c */
void set_init_1 __PARMS((void));
void set_init_2 __PARMS((void));
void set_init_3 __PARMS((void));
int do_set __PARMS((char_u *arg));
void set_options_bin __PARMS((int oldval, int newval));
int get_viminfo_parameter __PARMS((int type));
void check_options __PARMS((void));
void check_buf_options __PARMS((BUF *buf));
void free_string_option __PARMS((char_u *p));
void set_string_option __PARMS((char_u *name, int opt_idx, char_u *val, int dofree));
char_u *get_highlight_default __PARMS((void));
int makeset __PARMS((FILE *fd));
void clear_termoptions __PARMS((void));
void set_term_defaults __PARMS((void));
void comp_col __PARMS((void));
void win_copy_options __PARMS((WIN *wp_from, WIN *wp_to));
void buf_copy_options __PARMS((BUF *bp_from, BUF *bp_to, int entering, int always));
void set_context_in_set_cmd __PARMS((char_u *arg));
int ExpandSettings __PARMS((regexp *prog, int *num_file, char_u ***file));
int ExpandOldSetting __PARMS((int *num_file, char_u ***file));
char_u *file_pat_to_reg_pat __PARMS((char_u *pat, char_u *pat_end, int *allow_directories));
void do_autocmd __PARMS((char_u *arg, int forceit));
void do_doautocmd __PARMS((char_u *arg));
int apply_autocmds __PARMS((int event, char_u *fname, char_u *fname_io));
char_u *set_context_in_autocmd __PARMS((char_u *arg, int doautocmd));
int ExpandEvents __PARMS((regexp *prog, int *num_file, char_u ***file));
int has_format_option __PARMS((int x));
int shortmess __PARMS((int x));
