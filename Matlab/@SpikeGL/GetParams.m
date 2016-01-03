% params = GetParams( myobj )
%
%     Get the most recently used run parameters.
%     These are a struct of name/value pairs.
%
function ret = GetParams( s )

    ret = struct();
    res = DoGetResultsCmd( s, 'GETPARAMS' );

      % res is a cell array, each cell containing a string of form
      % '<parameter name> = <parameter value>'
      % Parameter names are sequences of word characters [a-z_A-Z0-9]
      % some examples of parameter values:
      %   3
      %   0.5
      %   -1
      %   false
      %   0:1
      %   0=1,1=2
      %   Dev1
      %   C:/Documents and Settings/labadmin/My Documents/data.bin

    for i = 1:length( res )

        % (?<xxx>expr) captures token matching expr and names it 'xxx'

        pair = ...
        regexp( res{i}, ...
        '^\s*(?<name>\w+)\s*=\s*(?<value>.*)\s*$', 'names' );

        % pair is a struct array with at most one element. If there
        % is an element, then pair.name is the (string) name, and
        % pair.value is the (string) value.

        if( ~isempty( pair ) )

            % If looks like a double, convert to double.

            dblpat = ...
            regexp( pair.value, ...
            '^\s*[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?\s*$', ...
            'once' );

            if( isempty( dblpat ) )
              % store the value as a string
              ret.(pair.name) = pair.value;
            else
              % convert to double
              ret.(pair.name) = str2double( pair.value );
            end
        end
    end
end
