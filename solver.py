from z3 import *

# State = Datatype('State')
# State.declare('WAITING_FOR_SHUTTLE_REQUEST')
# State.declare('TO_PICK_UP')
# State.declare('WAITING_FOR_PICK_UP_CONFIRMATION')
# State.declare('TO_DROP_OFF')
# State.declare('WAITING_FOR_DROP_OF_CONFIRMATION')
# State.declare('TO_FINAL_WAY_POINT')
# State = State.create()
#
# this_current_state_s0 = State.TO_PICK_UP
# this_current_state_s1 = State
# this_current_state_s2 = State
#
# s = Solver()
# s.add(
#     this_current_state_s1 == State.WAITING_FOR_DROP_OF_CONFIRMATION,
#     Not(this_current_state_s1 == State.WAITING_FOR_PICK_UP_CONFIRMATION)
# )
# print(s.check())
# print(s.model())
#
this_enable_route_planning_s0 = Bool('this_enable_route_planning_s0')
nearest_empty = Bool('nearest_empty')
update_needed_updateRoutePlan_s0 = Bool('update_needed_updateRoutePlan_s0')
this_first_init_path = Bool('this_first_init_path')
this_ref_gps_updated_ = Bool('this_ref_gps_updated_')


this_ref_gps_flag__s0 = True
update_needed_updatePath_s4 = False
update_needed_updatePath_s3 = update_needed_updatePath_s4 + False
update_needed_updatePath_s2 = update_needed_updatePath_s3 + True
update_needed_updatePath_s1 = update_needed_updatePath_s2 + True
update_needed_updatePath_s0 = update_needed_updatePath_s1 + False

s = Solver()
s.add(
    this_ref_gps_flag__s0,
    this_enable_route_planning_s0,
    this_ref_gps_flag__s0,
    nearest_empty,
    update_needed_updateRoutePlan_s0,
    update_needed_updatePath_s0 != False
)

print(s.check())
print(s.model())