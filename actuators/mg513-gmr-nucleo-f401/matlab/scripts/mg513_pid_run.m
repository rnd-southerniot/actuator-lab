function u = mg513_pid_run(sp, meas, dt, kp, ki, kd)
%MG513_PID_RUN  Run MG513_PID_FCN over a whole (sp, meas) sequence.
%   u = MG513_PID_RUN(sp, meas, dt, kp, ki, kd) applies the firmware-faithful
%   PID to prescribed setpoint/measurement vectors (open-loop block test) and
%   returns the output vector. Used by the MATLAB-vs-SIL equivalence check.
    g  = mg513_pid_gains(kp, ki, kd);
    st = mg513_pid_state();
    n  = numel(sp);
    u  = zeros(1, n);
    for k = 1:n
        [uk, st] = mg513_pid_fcn(sp(k), meas(k), dt, g, st);
        u(k) = uk;
    end
end
