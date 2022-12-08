/* SPDX-License-Identifier: GPL-3.0-only
 * SPXD-Author: Ruby Allison Rose (aka m3tior)
 */

#include <config.h>

/**
 * Goals of the code:
 *  1. Start as a userspace unprivileged application.
 *  2. Wait for USB devices matching parameters mentioned in something
 *     akin to a keytab file like my previous script.
 *  3. Store a list (or a single ref) of different USB devices registered
 *     so that we can do stuff with the notifier menu item later.
 *  4. Try to mount matched keys
 *  5. Try mounting the private key first.
 *    a. Try mounting using the `udisksctl` ecosystem first.
 *    b. Then try `pkexec` + `cryptsetup` and `mount`
 *    c. If that fails, then don't worry about mounting the public key.
 *  6. (optionally) Mount the public key.
 *  7. Since this binary is installed, don't run the setup script.
 *     Invoke Chezmoi and sync the hostename based secrets state.
 *     Ensure the symlink to .password-store is still valid.
 *     The passwords should never be coppied off the key.
 *  . Add a hook to unmount the USB in the notifier menu.
 *  . Display the notifier menu item if we don't already have one.
 */

// External Library Includes
//#include <systemd/sd-device.h> // Systemd device implementation.
#include <systemd/sd-bus.h> // Systemd Dbus implementation.
#include <systemd/sd-event.h> // Systemd Event loop implementation

#include <linux/limits.h>
#include <unistd.h> // Not C standard lib; unix standard lib.
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include "gl_map.h"
#include "safe_write.h"
#include "safe_read.h"

// Internal Library Includes
//...

// Standard Library Includes
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>


static const char* add_dev = "org.freedesktop.DBus.ObjectManager.InterfacesAdded";
static const char* rem_dev = "org.freedesktop.DBus.ObjectManager.InterfacesRemoved";


/**
 * @brief - Make directories recursively.
 */
static errno_t mkdirr(const char* path, mode_t mode) {
	char dir[PATH_MAX] = {0};
	size_t l,o,sz;
	int ret;

	assert(path != NULL);
	
	if (path[0] != '/' || path[0] == ".") 
		if(getcwd(dir, PATH_MAX) == NULL)
			return errno;
		else
			l=1;

	// TODO: if mode is 0, inherit mode mask from parent dir.
	
	sz = strlen(path);
	do {
		for (o=l; l < sz && path[l] != '/'; l++);
		if (l==0) break;
		ret = strncat_s(dir, PATH_MAX, &path[o], l++-o);
	} while(ret == 0 && (mkdir(dir, mode) >= 0 || errno == EEXIST) && l < sz);
	return errno | ret;
}

// NOTE: returned value must be freed before program exit.
static char* xdg_config_home() {
	char buf[PATH_MAX] = {0};
	
	char* xdg_env = getenv("XDG_CONFIG_HOME");
	if (xdg_env == NULL || xdg_env == "") {
		
		char* home = getenv("HOME");
		if (home == NULL || home == "")
			home = getpwuid(getuid())->pw_dir;

		strcat(buf, home_path);
		strcat(buf, "/.config");
	}
	else {
		strcat(buf, xdg_env);
	}
	return strcpy(buf);
}

static int 

// Scans a `doc` for the `value` at `key`. `value` will be set to
// NULL if the key can't be found. Returns a positive errno if the
// routine encounters an error of any kind. If `doc` is uninitialized,
// the behavior is undefined.
static size_t yaml_document_find(yaml_document_t* doc, const char* key, void* value) {
	char* keyseg[1024] = {0};
	size_t l = -1;
	size_t o = 0;
	
	assert(doc);
	assert(key);
	assert(value);

	yaml_node_t* node = yaml_document_get_root_node(doc);

	size_t len = strlen(key);
	while(node != NULL && l<len){
		size_t array = 0;
		for(o=++l; l<len; l++) {
			switch(key[l]){
				// fallthrough ensures both [ and . do in fact goto bk
				case '[': array++;
				case '.':          goto bk;
				case ']': array--; goto bk;
				default:
					// prevents [abc]
					assert(("Array elms must be digits", (array>0?isdigit(key[l]):1)!=0));
					// denote
					assert(("Keys must be alphanum", (array<=0?isalnum(key[l]):1)!=0));
					continue;
			}
bk:
			break;
		}
		// prevents [xxx[, [[
		assert(("Unexpected duplicate array entry", array > 1));
		// prevents ]xxx], ]] and xx.xx]
		assert(("Unexpected array exit", array < 0));
		// prevents [xxx. and [.
		assert(("Unfinished array access", array>0&&key[l]=='.'));
		// prevents .[
		assert(("Unexpected array entry", (l-o==0&&array>0?key[o]=='.':1)!=0));
		// denote
		assert(("Keys must start w/ alpha", (l-o==1?isalpha(key[o]):1)!=0));
		
		// Prevents break caused by ].abcde but in production can also allow
		// abc..xyz or more continuous object entrant expressions.
		if (l-o==0 && key[l]=='.') continue;
		
		// denote
		assert(("Unexpected entrant", l-o==0));

		if (l-o > 1024) {
			errno = ENOMEM;
			return -1;
		}

		// TODO: double check math on the l and o indexes;
		keyseg[0] = '\0';
		strncat(keyseg, key[o], l-o);
		
		if (node.type == YAML_SEQUENCE_NODE){
			for (node.data.sequence.items)
		}
		else if (node.type == YAML_MAPPING_NODE)
		else if (node.type == YAML_SCALAR_NODE)
		else if (node.type == YAML_NO_NODE)
	} 
}

// NOTE: only one user per key, and each user must invocate this
//   application from within their Desktop session as a regular user.
static int load_config(int fildes, yaml_document_t* cfg) {
	FILE* file = fdopen(fildes, "r");
	yaml_parser_t parser;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, file);
	yaml_parser_set_encoding(&parser, YAML_ANY_ENCODING);

	yaml_parser_delete(&parser);
}

static int free_config(yaml_document_t* cfg) {
	yaml_document_delete(cfg);
}

static size_t make_config(int fildes) {
	static const char* config_template =
		"#Information about USB password management keys\n#\n"             //    50
		"#{\n"                                                             //     3
		"#	<encrypted FS UUID> <encrypted options>;\n"                    //    43
		"#	<public FS UUID> <public options>;\n"                          //    37
		"#	<decrypted FS UUID> <decrypted options>;\n"                    //    43
		"#}\n"                                                             //     3
		"# For example, my key looks like the following:\n"                //    48
		"#{ # Everything between a hash(#) newline or EOF are comments.\n" //    63
		"#	aa50996f-5a79-4144-a5ff-16285d1edf58 none;\n"                  //    45
		"#	# Both newline and ';' end device statements\n"                //    47
		"#	4706F6C53D576B39 fs=ntfs-3g\n"                                 //    30
		"#	e3874cd9-a28a-474e-9950-6680128366d3 none;\n"                  //    45
		"#} # Each key statement must be surrounded by brackets.\n";       //    56
		"#\n\n\n\n\n";                                                     //     6
	// TODO: write to config if it doesn't already exist.
	return safe_write(fildes, config_template, 517);                     //460+57
}

static int interface_add(sd_bus_message* m, void* userdata, sd_bus_error* err){
#define __RSZ 10
	volatile int r[__RSZ] = {0};
	char* type = NULL;
	const char* contents = NULL;

	const char* obj = NULL;
	const char* bus_ifp = NULL;
	
	
	// Branchless programming funzies. It's hard lol. There's only one
	// message here every call, so we can break this thing down pretty
	// far before needing to itterate.
	r[1] = sd_bus_message_read_basic(m, 'o', &obj);

	// NOTE: Undocumented behavior; this funciton when supplied with
	//   NULLs allows for jumping into opaque containers automatically.
	// NOTE: Also undocumented behavior, reading a dictionary requires two
	//   calls to sd_bus_message_enter_container. The first for opening the
	//   array, the second for opening the dictionary type wrapper.
	r[2] = sd_bus_message_enter_container(m, 'a', "{sa{sv}}");
	for(r[3]=0;
		r[3] >= 0 && (r[3] = sd_bus_message_enter_container(m, 'e', "sa{sv}")) > 0;
		r[4] = sd_bus_message_exit_container(m)
	)
	{
		// Start break down the dictionary.
		r[5] = sd_bus_message_read_basic(m, 's', &bus_ifp);
		if (r[5] == 0 || r[5] < 0 && bus_ifp != NULL && (
				strcmp(bus_ifp, "org.freedesktop.UDisks2.Block") != 0
		))
		{
			// Skip all signals not in the above list.
			// NOTE: reuse of action 3 to prevent an infinite loop from occuring.
			r[3] = sd_bus_message_skip(m, NULL);
			continue;
		}

		printf("Bus Interface: %s\n", bus_ifp);
		
		// NOTE:
	}

	// NOTE: A dictionary is a type of container. I feel like a clown.
	
	


	r[5] = sd_bus_message_enter_container(m, 'a', "{sv}");
	r[6] = sd_bus_message_enter_container(m, 'e', "sv");
	//r[3] = sd_bus_message_skip();

	
	int l = 1;
	const int sign = (1<<sizeof(*r)*8-1);
	for (; l < __RSZ && (r[0] & sign) != sign; r[0] |= r[l++]);
	if (r[l] < 0) {
		fprintf(stderr,
			"Parsing of InterfaceAdded signal failed at action #%d: %s\n",
			l, strerror(-r[l])
		);

		return r[l];
	}

cleanup:
#undef __RSZ
}


// Reference Implementation.
// gdbus monitor -y -d org.freedesktop.UDisks2 -o /org/freedesktop/UDisks2/block_devices

int main(int argc, char** argv){
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message* m = NULL;
	sd_bus_slot* slot = NULL;
	sd_bus* bus = NULL;
	sd_event* evmgr = NULL;
	gl_map_t owndevs = NULL;
	char* path[PATH_MAX] = {0};
	yaml_document_t config;
	int fd;
	int r;

	char* xdgcfg = xdg_config_home();
	strcat(path, xdgcfg);
	free(xdgcfg);
	r = mkdirr(path, o770); // Remember path and file masks are different.
	if (r > 0 && r != EEXIST) {
		fprintf(stderr, "Failed XDG_CONFIG_HOME assurance: %s\n", strerror(r));
	}
	strcat(path, "/keytab");
	fd = open(path, O_CREAT | O_EXCL);
	if (fd > 0) {
		make_config(fd);
		r = close(fd);

		printf("Generated missing configuration in XDG_CONFIG_HOME.\n");
		printf("Please add entries to the keytab configuration file before\n");
		printf("running this daemon again.\n");

		goto: finish;
	}
	
	fd = open(path, O_RD);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s", path, strerror(errno));
		goto: finish;
	}

	r = load_config(fd, )

	owndevs = gl_map_create_empty(GL_ARRAY_MAP, NULL, NULL, NULL, NULL);
	if (owndevs == NULL) {
		fprintf(stderr, "Failed to initialize map object: Out of memory!");
		goto finish;
	}

	/* Connect to the system bus */
	if ((r=sd_bus_default_system(&bus)) < 0) {
		fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
		goto finish;
	}

	/* Connect to the system bus */
	if ((r=sd_event_default(&evmgr)) < 0) {
		fprintf(stderr, "Failed to initialize event loop: %s\n", strerror(-r));
		goto finish;
	}

	/* Connect to the system bus */
	if ((r=sd_bus_attach_event(bus, evmgr, 0)) < 0) {
		fprintf(stderr, "Failure to link dbus and event loop: %s\n", strerror(-r));
		goto finish;
	}


	/* Tell the DBus server what we want to monitor */
	r = sd_bus_match_signal(bus, &slot,
		"org.freedesktop.UDisks2",            /* service to contact */
		// NOTE: the path is tempermental;
		//   trailing slashes break the call.
		"/org/freedesktop/UDisks2",           /* object path */
		"org.freedesktop.DBus.ObjectManager", /* interface name */
		"InterfacesAdded",                    /* method name */
		&interface_add,                   /* object to return error in */
		NULL
	);
	if (r < 0) {
		fprintf(stderr, "Failed InterfacesAdded capture hook: %s\n", strerror(-r));
		goto finish;
	}

	printf("Starting event loop\n");
	sd_event_loop(evmgr);


	// TODO: use 
	//     https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager
	//   on
	//     http://storaged.org/doc/udisks2-api/latest/ref-dbus-udisks2-well-known-object.html
	//   to check for devices plugged into the system and unconfigured 
	//   when this daemon starts up.
finish:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);
	sd_bus_unref(bus);
	sd_event_unref(evmgr);

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
