#ifndef __HSCONF_INCLUDED__
#define __HSCONF_INCLUDED__

#include "hstypes.h"

#define HSPACE_CONFIG_FILE "space/hspace.cnf"

// A listing in the configuration array
typedef struct {
  char   *name;
  void   (*func)(int *, char *);
  int    *des;
}HCONF;

class CHSConf
{
public:

	// Methods
	CHSConf(void);
	BOOL LoadConfigFile(char *);

	// Attributes
	dbref 	afterworld;
	dbref 	space_wiz;

	BOOL	autostart;
	BOOL	log_commands;
	BOOL	autozone;
	BOOL	use_two_fuels;
	int		afterburn_fuel_ratio;
	int		detectdist;
	int		identdist;
	int		sense_hypervessels;
	int     max_sensor_range;
	int		afterburn_ratio;
	int		fuel_ratio;
	int 	min_flight_training;
	int    	max_universes;
	int     cyc_interval;
	int     train_interval;
	int		max_ships;
	int		max_active_ships;
	int		max_planets;
	int		max_land_speed;
	int		max_dock_dist;
	int		max_drop_dist;
	int		max_board_dist;
	int		max_dock_size;
	int		max_cargo_dist;
	int		max_strafe_dist;
	int     jump_wait;
	int		decay_ships;
	int     allow_strafing;
	int		notify_shipscan;
	int     forbid_puppets;
	int		use_comm_objects;
	int		cloak_opt_size;
	int		cloak_low_size;
	int		cloak_style;
	int		see_while_cloaked;
	int		fire_while_cloaked;
	int		jump_speed_multiplier;
	int		seconds_to_drop;
	char	univdb[256];
	char    territorydb[256];
	char	classdb[256];
	char	weapondb[256];
	char	objectdb[256];
	char    picture_dir[256];
	char	unit_name[32];

	/* Messages */
	char	ship_is_jumping[256];
	char	engines_activating[256];
	char	computer_activating[256];
	char	thrusters_activating[256];
	char	life_activating[256];
	char	reactor_activating[256];
	char	engines_offline[256];
	char	life_cut[256];
	char	jumpers_cut[256];
	char	engines_cut[256];
	char	sensors_cut[256];
	char	reactor_offline[256];
	char	speed_increase[256];
	char	speed_decrease[256];
	char	engine_reverse[256];
	char    engine_forward[256];
	char    speed_halt[256];
	char    ship_is_docking[256];
	char    ship_is_docked[256];
	char    ship_is_undocking[256];
	char	lift_off[256];
	char    begin_descent[256];
	char	landing_msg[256];
	char	end_jump[256];
	char    ship_jumps[256];
	char	afterburn_engage[256];
	char	afterburn_disengage[256];

protected:
	BOOL InputOption(char *, char *);

};

extern CHSConf HSCONF;

#endif
