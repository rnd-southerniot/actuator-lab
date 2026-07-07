function [u, st] = mg513_pid_fcn(sp, meas, dt, g, st)
%MG513_PID_FCN  Firmware-faithful PID step (matches pid.c / pid_py.py).
%   [u, st] = MG513_PID_FCN(sp, meas, dt, g, st) computes one control step of
%   the on-chip PID and returns the (clamped) output u and the updated state
%   struct st. Pure function (state passed in/out) so it is both unit-testable
%   and usable inside a Simulink MATLAB Function block (hold st as DiscreteState).
%
%   ISA/parallel PID with setpoint weighting (b on P, c on D), derivative on
%   measurement, filtered derivative (coefficient N), and back-calculation /
%   conditional-integration anti-windup — identical math to pid.c.
%
%   g : gains struct from MG513_PID_GAINS  (kp,ki,kd,N,b,c,out_min,out_max,i_min,i_max)
%   st: state struct from MG513_PID_STATE  (integral,prev_error,prev_derivative,
%                                           prev_measurement,prev_output)
%#codegen
    if dt <= 0.0
        u = st.prev_output;
        return;
    end

    e_p = g.b * sp - meas;          % P error (setpoint-weighted)
    e_i = sp - meas;                % I error (full)

    P = g.kp * e_p;

    % Back-calculation / conditional-integration anti-windup (pid.c Session-2 form)
    u_unsat = P + st.integral + st.prev_derivative;
    u_sat   = min(g.out_max, max(g.out_min, u_unsat));
    saturated  = (u_sat ~= u_unsat);
    winding_up = ((u_unsat - u_sat) * e_i) > 0.0;

    if ~(saturated && winding_up)
        st.integral = st.integral + g.ki * e_i * dt;
    end
    st.integral = min(g.i_max, max(g.i_min, st.integral));

    % Derivative on measurement, filtered (recurrence matches pid.c)
    d_raw = -(meas - st.prev_measurement) / dt;
    D = (g.kd * g.N * d_raw + st.prev_derivative) / (1.0 + g.N * dt);

    st.prev_derivative  = D;
    st.prev_measurement = meas;

    u = P + st.integral + D;
    u = min(g.out_max, max(g.out_min, u));

    st.prev_error  = e_i;
    st.prev_output = u;
end
