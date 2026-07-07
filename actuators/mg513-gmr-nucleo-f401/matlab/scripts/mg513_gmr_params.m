function p = mg513_gmr_params()
%MG513_GMR_PARAMS  Parameters for the MG513/GMR + DBH-12V + NUCLEO-F401RE rig.
%   First-order duty->RPM plant  G(s) = K/(tau*s + 1)  (motor shaft) plus the
%   on-chip control constants, matching the AS-FLASHED local firmware (baked
%   gains kp=0.0015/ki=0.01/kd=0, trap-vel fix, direction fix).
%
%   p = MG513_GMR_PARAMS() returns a struct AND pushes the scalar fields used by
%   the Simulink model into the base workspace (K, tau, dt, kp, ki, kd, N, b, c,
%   out_min, out_max, MOTOR_MAX_RPM, enc_lpf_alpha, ramp_rate) so blocks can
%   reference them by name. Templated on reference/qube-servo2-plant params.
%
%   K/tau are a PLACEHOLDER (see docs/MODELING.md): K from the DIAG open-loop
%   point (0.15 duty -> ~866 rpm), tau from the sibling MG513P30 accel step.
%   Clean plant-ID is blocked on the current firmware (closed-loop 100 Hz data,
%   friction-breakaway operating point, ~+/-30 rpm noise) -> needs the open-loop
%   1 kHz burst logger. Do NOT trust these for design without that upgrade.

    % --- First-order plant (duty -> motor-shaft RPM) --- PLACEHOLDER
    p.K   = 5773.0;   % DC gain [RPM at duty = 1.0]   (placeholder; see MODELING.md)
    p.tau = 0.08;     % mechanical time constant [s]  (placeholder; from sibling)

    % --- Drivetrain / encoder ---
    p.gear_ratio = 30;
    p.cpr_motor  = 2000;      % 500 PPR x4 quadrature
    p.MOTOR_MAX_RPM = 200;    % setpoint clamp (this telemetry unit)

    % --- Control loop (firmware) ---
    p.dt = 1e-3;              % 1 kHz TIM6 ISR
    p.ramp_rate = 200.0;      % SETPOINT_RAMP_RPM_PER_S
    % Encoder velocity IIR LPF (fc = 20 Hz at fs = 1 kHz) — matches encoder.c
    p.enc_lpf_alpha = 1.0 / (1.0 + 1000.0 / (2.0*pi*20.0));

    % --- Baked PID gains (as-flashed) ---
    p.kp = 0.0015;
    p.ki = 0.01;
    p.kd = 0.0;
    p.N  = 10.0;              % derivative filter coefficient
    p.b  = 1.0;              % P setpoint weight
    p.c  = 0.0;              % D setpoint weight (derivative on measurement)
    p.out_min = -1.0;
    p.out_max =  1.0;

    % Push scalars to base workspace for Simulink block references.
    fn = ["K","tau","dt","gear_ratio","cpr_motor","MOTOR_MAX_RPM", ...
          "ramp_rate","enc_lpf_alpha","kp","ki","kd","N","b","c", ...
          "out_min","out_max"];
    for k = 1:numel(fn)
        assignin('base', fn(k), p.(fn(k)));
    end
end
