function res = mg513_sim_closed(setpoint, dur, kp, ki, kd, K, tau, mode)
%MG513_SIM_CLOSED  Firmware-faithful closed-loop RPM prediction.
%   res = MG513_SIM_CLOSED(setpoint, dur, kp, ki, kd, K, tau, mode) simulates the
%   as-flashed control loop at 1 kHz and returns res.t / res.rpm_pred / res.out_pred.
%
%   Loop order matches src/tests/sil/sil_runner.run_rpm_step exactly:
%       setpoint --> rate limiter (200 RPM/s) --> PID(pid.c) --> clamp[-1,1]
%                --> first-order Euler plant --> encoder IIR LPF (fc=20 Hz)
%   The PID measures the *filtered* rpm from the previous tick. This is the curve
%   overlaid against live hardware in the HIL loop (K/tau from bench sys-ID).
%
%   mode: only "rpm" is implemented (position/cascade is a later phase).

    if nargin < 8 || isempty(mode); mode = "rpm"; end
    if ~strcmpi(mode, "rpm")
        error('mg513_sim_closed:mode', 'only mode="rpm" is implemented');
    end

    dt    = 1e-3;
    n     = round(dur / dt);
    alpha = 1.0 / (1.0 + 1000.0 / (2.0*pi*20.0));   % encoder LPF, fc=20 Hz
    rlrate = 200.0;                                  % SETPOINT_RAMP_RPM_PER_S

    g  = mg513_pid_gains(kp, ki, kd);
    st = mg513_pid_state();

    rpm      = 0.0;   % plant state (motor shaft RPM)
    rpm_filt = 0.0;   % IIR-filtered rpm (PID measurement)
    sp_rl    = 0.0;   % rate-limited setpoint

    t        = zeros(1, n);
    rpm_pred = zeros(1, n);
    out_pred = zeros(1, n);

    for k = 1:n
        % Rate limiter (slew-limit the setpoint)
        d  = setpoint - sp_rl;
        mx = rlrate * dt;
        d  = min(mx, max(-mx, d));
        sp_rl = sp_rl + d;

        % PID on the previous tick's filtered rpm
        [u, st] = mg513_pid_fcn(sp_rl, rpm_filt, dt, g, st);
        u = min(1.0, max(-1.0, u));

        % First-order plant, forward Euler: tau*dw/dt + w = K*u
        rpm = rpm + (K*u - rpm) / tau * dt;

        % Encoder velocity IIR filter
        rpm_filt = alpha*rpm + (1.0 - alpha)*rpm_filt;

        t(k)        = k * dt;
        rpm_pred(k) = rpm_filt;
        out_pred(k) = u;
    end

    res.t        = t;
    res.rpm_pred = rpm_pred;
    res.out_pred = out_pred;
end
