from z3 import *

this_first_init_path_s0 = Bool('this_first_init_path_s0')
this_ref_gps_updated__s0 = Bool('this_ref_gps_updated__s0')
this_enable_route_planning_s0 = Bool('this_enable_route_planning_s0')
update_needed_updateRoutePlan_s0 = Bool('update_needed_updateRoutePlan_s0')

this_ref_gps_flag__s0 = True
update_needed_updatePath_s3 = False
update_needed_updatePath_s2 = Or(update_needed_updatePath_s3, False)
update_needed_updatePath_s1 = Or(update_needed_updatePath_s2, this_first_init_path_s0)
update_needed_updatePath_s0 = Or(update_needed_updatePath_s1, this_ref_gps_updated__s0)

s = Solver()
s.add(
    And(this_ref_gps_flag__s0, Or(this_ref_gps_updated__s0, this_first_init_path_s0)),
    this_enable_route_planning_s0,
    Not(update_needed_updateRoutePlan_s0),
    Not(Not(update_needed_updatePath_s0)),
)

print(s.check())
print(s.model())
