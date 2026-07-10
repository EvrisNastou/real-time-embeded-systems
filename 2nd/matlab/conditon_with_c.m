clc;
clear;
close all;


clc;
clear;
close all;

% Ρύθμιση του URI του Jetstream
uri = 'wss://jetstream1.us-east.bsky.network/subscribe?wantedCollections=app.bsky.feed.post';
disp('[MATLAB] Προσπάθεια σύνδεσης στο WebSocket...');

try
    % Δημιουργία σύνδεσης μέσω Python
    ws = py.websocket.create_connection(uri);

    % ΒΑΖΟΥΜΕ TIMEOUT: Αν δεν έρθει μήνυμα σε 0.1 δευτερόλεπτα, να προχωράει 
    % για να μπορούμε να τυπώνουμε τα στατιστικά μας στην ώρα τους!
    ws.settimeout(0.1); 

    disp('[MATLAB] Η σύνδεση επιτεύχθηκε! Ξεκινάει η καταγραφή...');

    % Αρχικοποίηση μετρητών (όπως στο app_stats της C)
    commits = 0;
    identities = 0;
    accounts = 0;
    infos = 0;
    cycle_count = 0;

    % Ξεκινάμε το χρονόμετρο
    t_start = tic;
    last_tick = 0;

    while true
        try
            % Λήψη μηνύματος
            result = ws.recv();
            json_str = char(result);

            % Αποκωδικοποίηση JSON
            data = jsondecode(json_str);

            % Ελέγχουμε τι είδους μήνυμα ήρθε και αυξάνουμε τον αντίστοιχο μετρητή
            if isfield(data, 'kind')
                switch data.kind
                    case 'commit'
                        commits = commits + 1;
                    case 'identity'
                        identities = identities + 1;
                    case 'account'
                        accounts = accounts + 1;
                    case 'info'
                        infos = infos + 1;
                end
            end

        catch ME
            % Αν το σφάλμα είναι απλά timeout (πέρασαν 0.1s χωρίς μήνυμα), το αγνοούμε
            if contains(string(ME.message), 'timed out')
                % Συνεχίζουμε κανονικά στο loop
            else
                % Αν είναι άλλο σοβαρό σφάλμα, σταματάμε
                disp('Σφάλμα κατά τη λήψη/αποκωδικοποίηση:');
                disp(ME.message);
                break;
            end
        end

        % --- ΛΟΓΙΚΗ MONITOR ---
        % Ελέγχουμε αν πέρασε 1 δευτερόλεπτο
        current_time = toc(t_start);
        if current_time - last_tick >= 1.0
            cycle_count = cycle_count + 1;

            % Τυπώνουμε τα στατιστικά όπως ακριβώς και στη C
            fprintf('\n[MATLAB Monitor] --- Tick %d ---\n', cycle_count);
            fprintf('  Δεδομένα : Commits: %d | Identities: %d | Accounts: %d | Infos: %d\n', ...
                commits, identities, accounts, infos);

            % Μηδενίζουμε τους μετρητές για το επόμενο δευτερόλεπτο
            commits = 0;
            identities = 0;
            accounts = 0;
            infos = 0;

            % Ανανεώνουμε τον χρόνο του τελευταίου "Tick"
            last_tick = last_tick + 1.0; 
        end
    end

catch ME
    disp('[MATLAB] Σφάλμα σύνδεσης:');
    disp(ME.message);
end

% Κλείσιμο σε περίπτωση που σπάσει το loop (π.χ. με Ctrl+C)
if exist('ws', 'var')
    ws.close();
    disp('[MATLAB] Η σύνδεση έκλεισε.');
end