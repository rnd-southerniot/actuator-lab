function res = mg513_sim_closed(setpoint, dur, kp, ki, kd, K, tau, mode)
%MG513_SIM_CLOSED  Firmware-faithful closed-loop RPM prediction.
%   res = MG513_SIM_CLOSED(setpoint, dur, kp, ki, kd, K, tau, mode) simulates the
%   as-flashed control loop at 1 kHz and returns res.t / res.rpm_pred / res.out_pred.
%
%   Loop order matches the as-flashed firmware exactly:
%       setpoint --> rate limiter (200 RPM/s) --> PID(pid.c) --> clamp[-1,1]
%                --> first-order Euler plant --> model-based velocity OBSERVER
%   The observer (encoder.c) predicts velocity from the previous tick's applied
%   duty via the baked plant (OBS_K, OBS_TAU, Coulomb) and corrects with the
%   measurement at gain OBS_L. The PID measures the observer estimate from the
%   previous tick. This is the curve overlaid against live hardware in the HIL
%   loop (K/tau are the *plant* under test; OBS_* are the firmware's baked model).
%
%   mode: only "rpm" is implemented (position/cascade is a later phase).

    if nargin < 8 || isempty(mode); mode = "rpm"; end
    if ~strcmpi(mode, "rpm")
        error('mg513_sim_closed:mode', 'only mode="rpm" is implemented');
    end

    dt    = 1e-3;
    n     = round(dur / dt);
    rlrate = 200.0;                                  % SETPOINT_RAMP_RPM_PER_S

    % Model-based velocity observer — baked firmware constants (config.h).
    OBS_K   = 10766.0;   % predictor DC gain [rpm/duty]
    OBS_TAU = 0.067;     % predictor time constant [s]
    OBS_C   = 306.0;     % Coulomb breakaway offset [rpm]
    OBS_L   = 0.10;      % correction gain

    % Plant Coulomb friction + stiction breakaway, in DUTY space (so it is
    % independent of the rpm-unit scaling of K). Below U_BD the shaft cannot
    % overcome static friction (stuck); above it, kinetic Coulomb subtracts an
    % equivalent duty. Identified from the clean open-loop CMD_SET_DUTY map
    % (breakaway ~0.028 duty; Coulomb 306 rpm / K=10766).
    U_BD    = 0.028;     % breakaway duty (static + kinetic Coulomb, duty-equiv)

    g  = mg513_pid_gains(kp, ki, kd);
    st = mg513_pid_state();

    rpm      = 0.0;   % plant state (motor shaft RPM)
    rpm_filt = 0.0;   % observer velocity estimate (PID measurement)
    sp_rl    = 0.0;   % rate-limited setpoint
    u_prev   = 0.0;   % previous tick's applied duty (observer predictor input)

    t        = zeros(1, n);
    rpm_pred = zeros(1, n);
    out_pred = zeros(1, n);

    for k = 1:n
        % Rate limiter (slew-limit the setpoint)
        d  = setpoint - sp_rl;
        mx = rlrate * dt;
        d  = min(mx, max(-mx, d));
        sp_rl = sp_rl + d;

        % PID on the previous tick's observer estimate
        [u, st] = mg513_pid_fcn(sp_rl, rpm_filt, dt, g, st);
        u = min(1.0, max(-1.0, u));

        % First-order plant with Coulomb friction + stiction dead-zone,
        % forward Euler: tau*dw/dt + w = K*u_eff.
        if abs(u) <= U_BD && abs(rpm) < 1.0
            u_eff = 0.0;                    % stuck: below static breakaway
        else
            u_eff = u - sign(u) * U_BD;     % kinetic Coulomb duty offset
        end
        rpm = rpm + (K*u_eff - rpm) / tau * dt;

        % Model-based velocity observer (predictor–corrector). Measurement is the
        % (noiseless in-model) plant velocity; predictor uses u_prev. Friction-
        % compensated affine steady state, clamped to the stiction dead-zone.
        au  = abs(u_prev);
        wss = OBS_K*au - OBS_C;
        if wss < 0.0,    wss = 0.0;  end
        if u_prev < 0.0, wss = -wss; end
        w_pred   = rpm_filt + (wss - rpm_filt) * (dt / OBS_TAU);
        rpm_filt = w_pred + OBS_L * (rpm - w_pred);
        u_prev   = u;

        t(k)        = k * dt;
        rpm_pred(k) = rpm_filt;
        out_pred(k) = u;
    end

    res.t        = t;
    res.rpm_pred = rpm_pred;
    res.out_pred = out_pred;
end
