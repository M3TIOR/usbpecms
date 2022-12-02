/* SPDX-License-Identifier: GPL-3.0-only
 * SPXD-Author: Ruby Allison Rose (aka m3tior)
 */

#include <config.h>

/**
 * TODO: investigate the following docs:
 *  https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/
 * NOTE: for inpsecting the Dbus system states, use `d-feet`, it's good!
 *  I already discovered that the global org.freedesktop.StatusNotifierWatcher
 *  singleton instance resides in the session bus under the object
 *  > /StatusNotifierWatcher
 *
 * XXX: The docs for dbus are actually at this address; they're just convoluted
 *  https://dbus.freedesktop.org/doc/api/html/index.html
 * NOTE: Don't use udev, use sd-device; udev is outdated.
 * XXX: Here's the docs for sd-device:
 *  https://www.freedesktop.org/software/systemd/man/sd-device.html
 * NOTE: Using sd-device will mean this binary requires systemd for the time
 *  being, but that's okay since for right now most devices that this caters to
 *  are personal devices which are more likely to use systemd than any other
 *  init system. Open to expansion using raw linux kernel call implementation
 *  at a later date.
 *
 * NOTE: MOHTERFUCKER oiJAOISjFOAIHVOIHWEVA
 *   Both libudev and libdbus have been integrated into systemd as the default
 *   effect is slowly pushing more people toward monolithic design. Not saying
 *   this is necessarily a bad thing, though the attack surface topology is
 *   immense. Since they've both been integrated into systemd, their new home
 *   is in libsystemd.
 *    libudev has become <systemd/sd-device.h>
 *    libdbus has become <systemd/sd-bus.h>
 *
 *   The DBus API seems fairly well made. But both are extremely poorly
 *   documented at this point. No sign of inline-docs or doxygen drops anywhere.
 *   For the DBus API this is the best link:
 *     https://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html
 *   which demonstrates AN ACTUAL EXAMPLE of how to use the new API.
 *   It's far form comprehensive. But at least it's something to go by.
 *
 * XXX: OIJEOIHOIHVOIH WHAT THE ACTUAL FUCKJOAHPIHVPIAHPVIHPAIHVPIAHV
 *   udisks is a part of systemd now too. And guess what, it supports A
 *   FUCKING DBUS IPC PROTOCOL. GOD DAMMIT!
 *     http://storaged.org/doc/udisks2-api/latest/
 *   So while IG I can't use sd-device.h for anything useful, I can at least
 *   breath a sign of relief that I won't have to dig any deeper.
 *   Apparently libdbus isn't dead (technically), It was written when the
 *   specification for DBus was, so it's API is a bit terse, but it's still
 *   going to be supported for a long while. I could use that API if
 *   further portability is desired. Though udisks2 is a part of Systemd
 *   So we'd need to find out what systems could stand-in for udisks2 on
 *   other systems.
 */

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
#include <unistd.h> // Not C standard lib; unix standard lib.
#include "gl_map.h"


// Internal Library Includes
//...

// Standard Library Includes
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>


static const char* add_dev = "org.freedesktop.DBus.ObjectManager.InterfacesAdded";
static const char* rem_dev = "org.freedesktop.DBus.ObjectManager.InterfacesRemoved";


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
				strcmp(bus_ifp, "org.freedesktop.UDisks2.Block") != 0 ||
				strcmp(bus_ifp, "org.freedesktop.UDisks2.Encrypted") != 0
		))
		{
			// Skip all signals not in the above list.
			// NOTE: reuse of action 3 to prevent an infinite loop from occuring.
			r[3] = sd_bus_message_skip(m, NULL);
			continue;
		}

		printf("Bus Interface: %s\n", bus_ifp);
		
		
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
	const char* path;
	int r;

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

	/* Become set the connection to monitor mode */
	//if ((r=sd_bus_set_monitor(bus, true)) < 0) {
	//	fprintf(stderr, "Failed to make bus a monitor: %s\n", strerror(-r));
	//	goto finish;
	//}
	
//	/* Tell the DBus server what we want to monitor */
//	r = sd_bus_call_method(bus,
//		"org.freedesktop.DBus",               /* service to contact */
//		// NOTE: the path is tempermental;
//		//   trailing slashes break the call.
//		"/org/freedesktop/UDisks2",           /* object path */
//		"org.freedesktop.DBus",               /* interface name */
//		"AddMatch",                           /* method name */
//		&error,                               /* object to return error in */
//		&m,                                   /* return message on success */
//		// If you're confused about this see:
//		//   https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
//		"s",                                  /* input signature */
//		"type='signal'"
//	);
//	if (r < 0) {
//		fprintf(stderr, "Failed to issue AddMatch: %s\n", error.message);
//		goto finish;
//	}
//	r = sd_bus_message_read(m, "");
//	if (r < 0) {
//		fprintf(stderr, "Response error: %s\n", strerror(-r));
//		goto finish;
//	}


	/* Tell the DBus server what we want to monitor */
//	r = sd_bus_call_method(bus,
//		"org.freedesktop.UDisks2",            /* service to contact */
//		"/org/freedesktop/UDisks2",           /* object path */
//		"org.freedesktop.DBus.ObjectManager", /* interface name */
//		"GetManagedObjects",                  /* method name */
//		&error,                               /* object to return error in */
//		&m,                                   /* return message on success */
//		// If you're confused about this see:
//		//   https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
//		""                               /* input signature */
//	);
//	if (r < 0) {
//		fprintf(stderr, "Failed to issue GetManagedObjects: %s\n", error.message);
//		goto finish;
//	}  
//	r = sd_bus_message_verify_type(m, NULL, add_dev);
//	if (r > 0) {
//		printf("Interface Added!");
//	}/ 
//	else if ((r=sd_bus_message_verify_type(m, NULL, rem_dev)) > 0) {
//		printf("Interface Removed!");
//	}
//	else if (r < 0) {} // Skip errors to be processed by following branch
//	else {
//		printf("Skipped Unknown Message!");
//	}

//	while(true) {
//	/* Parse the response message */
//	r = sd_bus_message_read(m, "s", &path);
//	if (r < 0) {
//		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
//		goto finish;
//	}
//	 
//	printf("%s", path);
//	}

finish:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);
	sd_bus_unref(bus);
	sd_event_unref(evmgr);

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
