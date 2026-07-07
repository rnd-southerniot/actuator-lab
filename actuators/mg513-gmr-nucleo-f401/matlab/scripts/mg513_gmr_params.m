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
%   K/tau are BENCH-IDENTIFIED (2026-07-07) from an open-loop duty step via the
%   CMD_SET_DUTY firmware command: steady-state slope of the rpm-vs-duty map
%   (R2>0.99, clean) gives K; a first-order fit of the 0->0.2 duty step gives tau
%   (R2=0.9925). Small-signal linear model for control; the plant also has a
%   Coulomb breakaway ~0.028 duty (a DC offset, not in this incremental model).

    % --- First-order plant (duty -> motor-shaft RPM) --- bench-identified
    p.K   = 10766.0;  % DC gain [RPM/duty] — steady-state slope (open-loop map)
    p.tau = 0.067;    % mechanical time constant [s] — open-loop step fit

    % --- Drivetrain / encoder ---
    p.gear_ratio = 30;
    p.cpr_motor  = 2000;      % 500 PPR x4 quadrature
    p.MOTOR_MAX_RPM = 200;    % setpoint clamp (this telemetry unit)

    % --- Control loop (firmware) ---
    p.dt = 1e-3;              % 1 kHz TIM6 ISR
    p.ramp_rate = 200.0;      % SETPOINT_RAMP_RPM_PER_S
    % Velocity estimator (as-flashed firmware): model-based OBSERVER — predicts
    % velocity from the commanded duty via the identified plant (friction-
    % compensated affine K·u − Coulomb, relaxing with tau_obs), corrects with the
    % raw encoder at gain L. Low noise at the plant-tau lag floor (no filter lag).
    % See encoder.c / MODELING.md. (enc_lpf_alpha retained for the legacy design
    % .slx, which still models the superseded fc=20 Hz IIR.)
    p.obs_K   = 10766.0;      % predictor DC gain [rpm/duty]
    p.obs_tau = 0.067;        % predictor time constant [s]
    p.obs_C   = 306.0;        % Coulomb breakaway offset [rpm]
    p.obs_L   = 0.10;         % correction gain (HIL-tunable)
    p.enc_lpf_alpha = 1.0 / (1.0 + 1000.0 / (2.0*pi*20.0));  % legacy IIR (design .slx)

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
          "ramp_rate","obs_K","obs_tau","obs_C","obs_L","enc_lpf_alpha", ...
          "kp","ki","kd","N","b","c","out_min","out_max"];
    for k = 1:numel(fn)
        assignin('base', fn(k), p.(fn(k)));
    end
end
