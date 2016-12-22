/* radare - LGPL - Copyright 2009-2016 - nibble, pancake */

#include <r_types.h>
#include <r_bin.h>

R_API int r_bin_addr2line(RBin *bin, ut64 addr, char *file, int len, int *line) {
	RBinFile *binfile = r_bin_cur (bin);
	RBinObject *o = r_bin_cur_object (bin);
	RBinPlugin *cp = r_bin_file_cur_plugin (binfile);
	ut64 baddr = r_bin_get_baddr (bin);
	if (cp && cp->dbginfo) {
		if (o && addr >= baddr && addr < baddr + bin->cur->o->size) {
			if (cp->dbginfo->get_line) {
				return cp->dbginfo->get_line (
					bin->cur, addr, file, len, line);
			}
		}
	}
	return false;
}

R_API char *r_bin_addr2text(RBin *bin, ut64 addr, int origin) {
	char file[4096];
	int line;
	char *out = NULL, *out2 = NULL;
	char *file_nopath;

	{
		char *key = r_str_newf ("0x%"PFMT64x, addr);
		char *file_line = sdb_get (bin->cur->sdb_addrinfo, key, 0);
		if (file_line) {
			char *token = strchr (file_line, '|');
			if (token) {
				*token ++ = 0;
				int line = atoi (token);
				out = r_file_slurp_line (file_line, line, 0);
			}
			free (file_line);
		}
		free (key);
		if (out) {
			return out;
		}
	}
	file[0] = 0;
	if (r_bin_addr2line (bin, addr, file, sizeof (file), &line)) {
		if (bin->srcdir && *bin->srcdir) {
			char *slash = strrchr (file, '/');
			char *nf = r_str_newf ("%s/%s", bin->srcdir, slash? slash + 1: file);
			strncpy (file, nf, sizeof (file) - 1);
			free (nf);
		}
		// TODO: this is slow. must use a cached pool of mmaped files and line:off entries
		out = r_file_slurp_line (file, line, 0);
		if (!out) {
			return 0;
		}
		out2 = malloc ((strlen (file) + 64 + strlen (out)) * sizeof (char));
		if (origin > 1) {
			file_nopath = NULL;
		} else {
			file_nopath = strrchr (file, '/');
		}
		if (origin) {
			snprintf (out2, strlen (file) + 63 + strlen (out), "%s:%d%s%s",
				file_nopath? file_nopath + 1: file, line, *out? " ": "", out);
		} else {
			snprintf (out2, 64, "%s", out);
		}
		free (out);
	}
	return out2;
}

R_API char *r_bin_addr2fileline(RBin *bin, ut64 addr) {
	char file[1024];
	int line;
	char *out = NULL;
	char *file_nopath;

	if (r_bin_addr2line (bin, addr, file, sizeof (file) - 1, &line)) {
		int sz = strlen (file) + 10;
		file_nopath = strrchr (file, '/');
		out = malloc (sz);
		snprintf (out, sizeof (sz), "%s:%d",
			file_nopath? file_nopath + 1: file, line);
	}
	return out;
}
