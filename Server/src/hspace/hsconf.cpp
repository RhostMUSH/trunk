#include "hscopyright.h"
#include <stdlib.h>
#include "hsconf.h"
#include "hsutils.h"
#include <string.h>

CHSConf HSCONF;

void HSCfInt(int *, char *);
void HSCfStr(int *, char *);

HCONF hspace_conflist[] = {
    (char *) "afterburn_engage", HSCfStr,
    (int *) HSCONF.afterburn_engage,
    (char *) "afterburn_disengage", HSCfStr,
    (int *) HSCONF.afterburn_disengage,
    (char *) "lift_off", HSCfStr, (int *) HSCONF.lift_off,
    (char *) "landing_msg", HSCfStr, (int *) HSCONF.landing_msg,
    (char *) "end_jump", HSCfStr, (int *) HSCONF.end_jump,
    (char *) "begin_descent", HSCfStr, (int *) HSCONF.begin_descent,
    (char *) "ship_jumps", HSCfStr, (int *) HSCONF.ship_jumps,
    (char *) "life_cut", HSCfStr, (int *) HSCONF.life_cut,
    (char *) "jumpers_cut", HSCfStr, (int *) HSCONF.jumpers_cut,
    (char *) "engines_cut", HSCfStr, (int *) HSCONF.engines_cut,
    (char *) "sensors_cut", HSCfStr, (int *) HSCONF.sensors_cut,
    (char *) "reactor_offline", HSCfStr, (int *) HSCONF.reactor_offline,
    (char *) "engines_offline", HSCfStr, (int *) HSCONF.engines_offline,
    (char *) "ship_is_jumping", HSCfStr, (int *) HSCONF.ship_is_jumping,
    (char *) "speed_increase", HSCfStr, (int *) HSCONF.speed_increase,
    (char *) "speed_decrease", HSCfStr, (int *) HSCONF.speed_decrease,
    (char *) "engine_reverse", HSCfStr, (int *) HSCONF.engine_reverse,
    (char *) "engine_forward", HSCfStr, (int *) HSCONF.engine_forward,
    (char *) "speed_halt", HSCfStr, (int *) HSCONF.speed_halt,
    (char *) "ship_is_docking", HSCfStr, (int *) HSCONF.ship_is_docking,
    (char *) "ship_is_undocking", HSCfStr,
    (int *) HSCONF.ship_is_undocking,
    (char *) "ship_is_docked", HSCfStr, (int *) HSCONF.ship_is_docked,
    (char *) "unit_name", HSCfStr, (int *) HSCONF.unit_name,
    (char *) "classdb", HSCfStr, (int *) HSCONF.classdb,
    (char *) "objectdb", HSCfStr, (int *) HSCONF.objectdb,
    (char *) "weapondb", HSCfStr, (int *) HSCONF.weapondb,
    (char *) "univdb", HSCfStr, (int *) HSCONF.univdb,
    (char *) "territorydb", HSCfStr, (int *) HSCONF.territorydb,
    (char *) "picture_dir", HSCfStr, (int *) HSCONF.picture_dir,
    (char *) "engines_activating", HSCfStr,
	(int *) HSCONF.engines_activating,
    (char *) "life_activating", HSCfStr, (int *) HSCONF.life_activating,
    (char *) "thrusters_activating", HSCfStr,
	(int *) HSCONF.thrusters_activating,
    (char *) "reactor_activating", HSCfStr,
	(int *) HSCONF.reactor_activating,
    (char *) "computer_activating", HSCfStr,
	(int *) HSCONF.computer_activating,
    (char *) "detectdist", HSCfInt, &HSCONF.detectdist,
    (char *) "identdist", HSCfInt, &HSCONF.identdist,
    (char *) "cycle_interval", HSCfInt, &HSCONF.cyc_interval,
    (char *) "train_interval", HSCfInt, &HSCONF.train_interval,
    (char *) "autozone", HSCfInt, &HSCONF.autozone,
    (char *) "jump_wait", HSCfInt, &HSCONF.jump_wait,
    (char *) "max_ships", HSCfInt, &HSCONF.max_ships,
    (char *) "max_active_ships", HSCfInt, &HSCONF.max_active_ships,
    (char *) "max_planets", HSCfInt, &HSCONF.max_planets,
    (char *) "max_universes", HSCfInt, &HSCONF.max_universes,
    (char *) "max_strafe_dist", HSCfInt, &HSCONF.max_strafe_dist,
    (char *) "max_cargo_dist", HSCfInt, &HSCONF.max_cargo_dist,
    (char *) "max_board_dist", HSCfInt, &HSCONF.max_board_dist,
    (char *) "max_dock_size", HSCfInt, &HSCONF.max_dock_size,
    (char *) "max_land_speed", HSCfInt, &HSCONF.max_land_speed,
    (char *) "afterworld", HSCfInt, &HSCONF.afterworld,
    (char *) "autostart", HSCfInt, (int *) &HSCONF.autostart,
    (char *) "log_commands", HSCfInt, (int *) &HSCONF.log_commands,
    (char *) "notify_shipscan", HSCfInt, &HSCONF.notify_shipscan,
    (char *) "space_wiz", HSCfInt, &HSCONF.space_wiz,
    (char *) "max_dock_dist", HSCfInt, &HSCONF.max_dock_dist,
    (char *) "max_drop_dist", HSCfInt, &HSCONF.max_drop_dist,
    (char *) "allow_strafing", HSCfInt, &HSCONF.allow_strafing,
    (char *) "forbid_puppets", HSCfInt, &HSCONF.forbid_puppets,
    (char *) "decay_ships", HSCfInt, &HSCONF.decay_ships,
    (char *) "min_training", HSCfInt, (int *) &HSCONF.min_flight_training,
    (char *) "cloak_low_size", HSCfInt, &HSCONF.cloak_low_size,
    (char *) "cloak_opt_size", HSCfInt, &HSCONF.cloak_opt_size,
    (char *) "cloak_style", HSCfInt, &HSCONF.cloak_style,
    (char *) "fire_while_cloaked", HSCfInt, &HSCONF.fire_while_cloaked,
    (char *) "see_while_cloaked", HSCfInt, &HSCONF.see_while_cloaked,
    (char *) "use_comm_objects", HSCfInt, &HSCONF.use_comm_objects,
    (char *) "seconds_to_drop", HSCfInt, &HSCONF.seconds_to_drop,
    (char *) "use_two_fuels", HSCfInt, &HSCONF.use_two_fuels,
    (char *) "fuel_ratio", HSCfInt, &HSCONF.fuel_ratio,
    (char *) "max_sensor_range", HSCfInt, &HSCONF.max_sensor_range,
    (char *) "sense_hypervessels", HSCfInt,
    &HSCONF.sense_hypervessels,
    (char *) "afterburn_ratio", HSCfInt,
    &HSCONF.afterburn_ratio,
    (char *) "afterburn_fuel_ratio", HSCfInt,
    &HSCONF.afterburn_fuel_ratio,
    (char *) "jump_speed_multiplier", HSCfInt,
    &HSCONF.jump_speed_multiplier,
    NULL, NULL, NULL
};

CHSConf::CHSConf(void)
{

}

/*
 * Places a string from the configuration file into its storage location.
 */
void HSCfStr(int *des, char *value)
{
    strcpy((char *) des, value);
}

/*
 * Places a float from the configuration file into its storage location.
 */
void HSCfFloat(double *des, char *value)
{
    double fval;

    fval = atof(value);
    *des = fval;
}

/*
 * Places an int from the configuration file into its storage location.
 */
void HSCfInt(int *des, char *value)
{
    int ival;

    ival = atoi(value);
    *des = ival;
}

/* 
 * Takes an option string loaded from the config file and
 * figures out where to put it in hs_options.
 */
BOOL CHSConf::InputOption(char *option, char *value)
{
    HCONF *cfptr;

    for (cfptr = hspace_conflist; cfptr->name; cfptr++) {
	if (!strcasecmp(option, cfptr->name)) {
	    cfptr->func(cfptr->des, value);
	    return TRUE;
	}
    }
    return FALSE;
}

/* 
 * Opens the HSpace configuration file, loading in option strings and
 * sending them to hspace_input_option().
 */
BOOL CHSConf::LoadConfigFile(char *lpstrPath)
{
    FILE *fp;
    char tbuf[1024];
    char tbuf2[256];
    char option[256];
    char value[1024];
    char *ptr, *ptr2;

    hs_log((char *) "LOADING: HSpace configuration file.");

    hs_log(lpstrPath);
    /*
     * Open the configuration file for reading
     */
    fp = fopen(lpstrPath, "r");
    if (!fp) {
	hs_log((char *)
	       "ERROR: Unable to open hspace configuration file.");
	return FALSE;
    }

    /*
     * Read the entire file in.  Parse lines that have something in them
     * and don't begin with a '#'
     */
    while (fgets(tbuf, 256, fp)) {
	/*
	 * Truncate at the newline
	 */
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	/*
	 * Strip leading spaces
	 */
	ptr = tbuf;
	while (*ptr == ' ')
	    ptr++;

	/*
	 * Determine if the line is valid
	 */
	if (!*ptr || *ptr == '#')
	    continue;

	/*
	 * Parse out the option and value
	 */
	ptr2 = option;
	for (; *ptr && *ptr != ' ' && *ptr != '='; ptr++) {
	    *ptr2 = *ptr;
	    ptr2++;
	}
	*ptr2 = '\0';
	if (!*ptr) {
	    sprintf(tbuf2, "ERROR: Invalid configuration at option: %s",
		    option);
	    hs_log(tbuf2);
	    continue;
	}
	ptr2 = value;
	while (*ptr && (*ptr == ' ' || *ptr == '='))
	    ptr++;
	for (; *ptr; ptr++) {
	    *ptr2 = *ptr;
	    ptr2++;
	}
	*ptr2 = '\0';
	if (!InputOption(option, value)) {
	    sprintf(tbuf2, "ERROR: Invalid config option \"%s\"", option);
	    hs_log(tbuf2);
	}
    }
    fclose(fp);

    return TRUE;
}
