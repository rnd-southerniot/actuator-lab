function st = mg513_pid_state()
%MG513_PID_STATE  Zeroed PID state struct for MG513_PID_FCN (matches PID_Reset).
%#codegen
    st.integral        = 0.0;
    st.prev_error      = 0.0;
    st.prev_derivative = 0.0;
    st.prev_measurement= 0.0;
    st.prev_output     = 0.0;
end
