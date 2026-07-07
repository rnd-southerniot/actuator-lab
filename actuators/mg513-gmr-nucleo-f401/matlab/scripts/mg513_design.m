function out = mg513_design(targets)
%MG513_DESIGN  Model-based RPM PID design via pidtune on the plant.
%   out = MG513_DESIGN(targets) linearizes the duty->RPM plant G=K/(tau*s+1)
%   (params from MG513_GMR_PARAMS), reports the current baked closed-loop
%   metrics, and pidtune()s a PI controller at each target crossover in
%   `targets` (rad/s). Returns a struct array {wc,kp,ki,settle,overshoot}.
%   The firmware PID is parallel PI (duty = kp*e + ki*integral(e)), so
%   pidtune's Kp/Ki map directly to the on-chip kp/ki.
%
%   NOTE: plant K/tau are a PLACEHOLDER (see MODELING.md) until an open-loop
%   burst-logger sys-ID lands — treat designed gains as starting points to be
%   HIL-validated on hardware, not final.

    if nargin < 1 || isempty(targets); targets = [2 5 10 20]; end
    p = mg513_gmr_params();

    G  = tf(p.K, [p.tau 1]);                 % duty -> RPM
    C0 = pid(p.kp, p.ki);                     % current baked PI
    s0 = stepinfo(feedback(C0*G, 1));
    fprintf('plant (placeholder): K=%.0f rpm/duty  tau=%.3f s\n', p.K, p.tau);
    fprintf('CURRENT baked: kp=%.5f ki=%.4f -> settle=%.2fs overshoot=%.1f%%\n', ...
            p.kp, p.ki, s0.SettlingTime, s0.Overshoot);
    fprintf('%-10s %-9s %-8s %-9s %-9s\n', 'wc[rad/s]','kp','ki','settle[s]','overshoot%');

    out = struct('wc',{},'kp',{},'ki',{},'settle',{},'overshoot',{});
    for i = 1:numel(targets)
        C = pidtune(G, 'PI', targets(i));
        si = stepinfo(feedback(C*G, 1));
        out(i) = struct('wc',targets(i), 'kp',C.Kp, 'ki',C.Ki, ...
                        'settle',si.SettlingTime, 'overshoot',si.Overshoot);
        fprintf('  %-8.0f %-9.5f %-8.4f %-9.2f %-9.1f\n', ...
                targets(i), C.Kp, C.Ki, si.SettlingTime, si.Overshoot);
    end
end
