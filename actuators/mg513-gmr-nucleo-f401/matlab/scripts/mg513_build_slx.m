function mg513_build_slx(outpath)
%MG513_BUILD_SLX  Programmatically build the MG513/GMR Simulink design model.
%   MG513_BUILD_SLX(outpath) creates 'mg513_gmr_plant.slx' — a clean discrete
%   design model for interactive tuning (PID Tuner / pidtune) and future external
%   mode:  Step -> Rate Limiter -> Sum -> Discrete PID -> first-order plant ->
%   encoder LPF -> feedback, with To-Workspace logging.
%
%   This is the DESIGN artifact (stock tunable PID block; the plant is a zoh
%   discretization of K/(tau*s+1)). It is representative, not bit-exact — the
%   firmware-faithful predictor for hardware overlay is MG513_SIM_CLOSED.
%
%   Params (K,tau,dt,kp,ki,kd,N,MOTOR_MAX_RPM,ramp_rate,enc_lpf_alpha) come from
%   MG513_GMR_PARAMS (base workspace). Blocks reference them by name so the model
%   re-tunes when you change the base vars.

    if nargin < 1 || isempty(outpath)
        here = fileparts(mfilename('fullpath'));
        outpath = fullfile(here, '..', 'models', 'mg513_gmr_plant.slx');
    end
    p = mg513_gmr_params();  %#ok<NASGU> % also seeds base workspace

    name = 'mg513_gmr_plant';
    if bdIsLoaded(name); close_system(name, 0); end
    new_system(name);
    load_system(name);

    add = @(src, blk, pos) add_block(src, [name '/' blk], 'Position', pos);

    % --- Blocks ---------------------------------------------------------
    add('simulink/Sources/Step',                 'SP',   [30  100  60  130]);
    set_param([name '/SP'], 'Time','0', 'Before','0', 'After','sp_ref', ...
              'SampleTime','dt');

    add('simulink/Discontinuities/Rate Limiter', 'RL',   [100 100 140 130]);
    set_param([name '/RL'], 'RisingSlewLimit','ramp_rate', ...
              'FallingSlewLimit','-ramp_rate');

    add('simulink/Math Operations/Sum',          'Err',  [180 100 210 130]);
    set_param([name '/Err'], 'Inputs','+-');

    add('simulink/Discrete/Discrete PID Controller', 'PID', [250 95 320 145]);
    set_param([name '/PID'], ...
        'Controller','PID', 'TimeDomain','Discrete-time', ...
        'SampleTime','dt', 'P','kp', 'I','ki', 'D','kd', 'N','N', ...
        'LimitOutput','on', 'UpperSaturationLimit','out_max', ...
        'LowerSaturationLimit','out_min', 'AntiWindupMode','clamping');

    % Plant: zoh discretization of K/(tau*s+1)  ->  y[k]=ad*y[k-1]+bd*u[k-1]
    add('simulink/Discrete/Discrete Transfer Fcn', 'Plant', [370 95 450 145]);
    set_param([name '/Plant'], ...
        'Numerator','[0 K*(1-exp(-dt/tau))]', ...
        'Denominator','[1 -exp(-dt/tau)]', 'SampleTime','dt');

    % Encoder velocity IIR LPF: y[k]=alpha*x[k]+(1-alpha)*y[k-1]
    add('simulink/Discrete/Discrete Transfer Fcn', 'LPF',  [490 95 570 145]);
    set_param([name '/LPF'], ...
        'Numerator','[enc_lpf_alpha]', ...
        'Denominator','[1 -(1-enc_lpf_alpha)]', 'SampleTime','dt');

    add('simulink/Sinks/To Workspace',           'rpm',  [620 100 680 130]);
    set_param([name '/rpm'], 'VariableName','rpm', 'SaveFormat','Timeseries');
    add('simulink/Sinks/To Workspace',           'out',  [370 180 430 210]);
    set_param([name '/out'], 'VariableName','out', 'SaveFormat','Timeseries');

    add('simulink/Sinks/Scope',                  'Scope',[620 170 660 210]);

    % --- Wiring ---------------------------------------------------------
    add_line(name, 'SP/1',   'RL/1',  'autorouting','on');
    add_line(name, 'RL/1',   'Err/1', 'autorouting','on');
    add_line(name, 'Err/1',  'PID/1', 'autorouting','on');
    add_line(name, 'PID/1',  'Plant/1','autorouting','on');
    add_line(name, 'PID/1',  'out/1', 'autorouting','on');
    add_line(name, 'Plant/1','LPF/1', 'autorouting','on');
    add_line(name, 'LPF/1',  'rpm/1', 'autorouting','on');
    add_line(name, 'LPF/1',  'Err/2', 'autorouting','on');
    add_line(name, 'LPF/1',  'Scope/1','autorouting','on');

    % --- Solver: fixed-step discrete at dt ------------------------------
    set_param(name, 'SolverType','Fixed-step', 'Solver','FixedStepDiscrete', ...
              'FixedStep','dt', 'StopTime','3');

    % Ensure a default setpoint exists in base ws for standalone open.
    if ~evalin('base','exist(''sp_ref'',''var'')'); assignin('base','sp_ref',80); end

    outdir = fileparts(outpath);
    if ~isempty(outdir) && ~isfolder(outdir); mkdir(outdir); end
    save_system(name, outpath);
    close_system(name, 0);
    fprintf('built %s\n', outpath);
end
