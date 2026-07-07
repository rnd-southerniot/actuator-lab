function g = mg513_pid_gains(kp, ki, kd)
%MG513_PID_GAINS  Gains struct for MG513_PID_FCN, matching pid.c defaults.
%   g = MG513_PID_GAINS(kp,ki,kd) fills the fixed firmware parameters
%   (N=10, b=1, c=0, output/integrator clamp ±1.0) around the given gains.
%#codegen
    g.kp = kp;
    g.ki = ki;
    g.kd = kd;
    g.N  = 10.0;    % PID_DEFAULT_N (derivative filter coefficient)
    g.b  = 1.0;     % PID_DEFAULT_B (P setpoint weight)
    g.c  = 0.0;     % PID_DEFAULT_C (D setpoint weight -> derivative on measurement)
    g.out_max = 1.0;
    g.out_min = -1.0;
    g.i_max = 1.0;
    g.i_min = -1.0;
end
