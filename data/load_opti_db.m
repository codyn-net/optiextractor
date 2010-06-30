function db = load_opti_db(filename)
	db = struct();

	fd = fopen(char(filename), 'r');

	if fd == -1
		disp(['Could not open file: ' filename]);
		return
	end

	[ret, lineno, db] = parse_lines(fd, 0, 0, @parse_struct, db);
	fclose(fd);

	% Resize some matrices to more convenient 3d matrices
	popsize = db.population_size;
	iterations = db.iterations;

	if ~strcmp(db.job.optimizer, 'Systematic')
		db.parameter_values = permute(reshape(db.parameter_values, popsize, iterations, size(db.parameter_values, 2)), [2 1 3]);
		db.fitness_values = permute(reshape(db.fitness_values, popsize, iterations, size(db.fitness_values, 2)), [2 1 3]);
	end
end

function [ret, lineno, data] = parse_struct(fd, line, level, lineno, data)
	% Read field name
	origline = line;

	line = strtrim(line);
	idx = strfind(line, ':');
	ret = 1;

	if isempty(idx)
		error(['Invalid field without name on line ' num2str(lineno) ': ' origline]);
	end

	name = line(1:idx - 1);
	valuestr = strtrim(line(idx + 1:end));

	if strcmp(valuestr, '{}') || strcmp(valuestr, '[]')
		[ret, lineno, value] = parse_lines(fd, level + 1, lineno, @parse_matrix, eval(valuestr));
	elseif isempty(valuestr)
		[ret, lineno, value] = parse_lines(fd, level + 1, lineno, @parse_struct, struct());
	else
		value = eval(valuestr);
	end

	if ret
		data.(name) = value;
	else
		data.(name) = 0;
	end
end

function [ret, lineno, data] = parse_lines(fd, level, lineno, callback, data)
	tab = sprintf('\t');

	ret = 1;

	while ~feof(fd)
		start = ftell(fd);
		line = fgets(fd);

		lineno = lineno + 1;

		% Simply skip empty lines
		if isempty(strtrim(line))
			continue
		end

		curlevel = get_level(line, tab);

		if curlevel ~= level
			fseek(fd, start, -1);
			lineno = lineno - 1;
			break
		end

		[ret, lineno, data] = callback(fd, line, level, lineno, data);

		if ~ret
			break
		end
	end
end

function level = get_level(line, delimiter)
	level = 0;

	for i = 1:length(line)
		level = i - 1;

		if line(i) ~= delimiter
			break
		end
	end
end

function [ret, lineno, data] = parse_matrix(fd, line, level, lineno, data)
	ret = 1;
	data = vertcat(data, eval(line));
end
