function mg513_live_sim(setpoint, dur)
%MG513_LIVE_SIM  Live (wall-clock) view of the MG513/GMR closed-loop PID.
%   MG513_LIVE_SIM(setpoint, dur) runs the design model mg513_gmr_plant.slx at
%   1x real-time (Simulation Pacing) and streams the control story to the
%   Simulation Data Inspector (SDI), which updates live as the run advances:
%       setpoint · measured_rpm · error · control_duty
%
%   *** PURE SIMULATION *** — design plant (zoh K/(tau s+1)) + stock tunable
%   Discrete PID + the legacy encoder LPF. This is representative for watching
%   PID behavior; it is NOT bit-exact to firmware and NOT live hardware. To watch
%   the REAL controller on the board, use mcp/mg513_live_hil.py.
%
%   Non-destructive: configures the IN-MEMORY model only (signal logging + pacing).
%   It never saves the .slx and never changes any gains (kp/ki/kd from params).
%
%   Examples:
%     mg513_live_sim            % 80 rpm step, 3 s, 1x real-time
%     mg513_live_sim(150, 4)    % 150 rpm step, 4 s
%
%   Operator: run this, then watch the four signals fill in the Simulation Data
%   Inspector window over `dur` seconds. Re-run with a new setpoint to compare.

    if nargin < 1 || isempty(setpoint), setpoint = 80; end
    if nargin < 2 || isempty(dur),      dur = 3;       end

    mg513_gmr_params();                       % seed base ws (K,tau,gains,dt,...)
    assignin('base', 'sp_ref', setpoint);     % Step 'After' value = setpoint

    m = 'mg513_gmr_plant';
    if ~bdIsLoaded(m)
        load_system(fullfile(fileparts(mfilename('fullpath')), '..', 'models', [m '.slx']));
    end

    % --- mark the four control-story signals for logging (in-memory only) ---
    blks = {'RL', 'LPF', 'Err', 'PID'};
    nms  = {'setpoint', 'measured_rpm', 'error', 'control_duty'};
    for i = 1:numel(blks)
        ph = get_param([m '/' blks{i}], 'PortHandles');
        set_param(ph.Outport(1), 'DataLogging', 'on', ...
                  'DataLoggingNameMode', 'Custom', 'DataLoggingName', nms{i});
    end
    set_param(m, 'SignalLogging', 'on', 'SignalLoggingName', 'logsout', ...
              'StopTime', num2str(dur));

    % --- real-time pacing: 1 s of simulation per wall-clock second ----------
    set_param(m, 'EnablePacing', 'on', 'PacingRate', 1);

    % --- open SDI and start (non-blocking) so it streams live ---------------
    try, Simulink.sdi.view; catch, end        % no-op if running headless
    fprintf(['[mg513_live_sim] PURE SIM · %g rpm · %g s @ 1x real-time.\n' ...
             '  Watch: setpoint / measured_rpm / error / control_duty in the\n' ...
             '  Simulation Data Inspector. (Not firmware-exact, not hardware.)\n'], ...
            setpoint, dur);
    set_param(m, 'SimulationCommand', 'start');   % async; SDI updates live
end
