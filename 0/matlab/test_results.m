clc; clear; close all;

%% 1. ΟΡΙΣΜΟΣ ΠΑΡΑΜΕΤΡΩΝ
% ΠΡΟΣΟΧΗ: Αυτά τα νούμερα ΠΡΕΠΕΙ να είναι ΙΔΙΑ με αυτά που έβαλες στο main.c
N = 50000; % Αριθμός σημείων
d = 20;    % Διαστάσεις
k = 4;     % Κοντινότεροι γείτονες

fprintf('=== ΦΟΡΤΩΣΗ ΔΕΔΟΜΕΝΩΝ ΑΠΟ C ===\n');

%% 2. ΔΙΑΒΑΣΜΑ ΔΕΔΟΜΕΝΩΝ (ΑΠΟ ΤΑ .bin ΑΡΧΕΙΑ ΤΗΣ C)
% Διάβασμα του C_data.bin (Ο πίνακας δεδομένων)
fid = fopen('C_data.bin', 'r');
if fid == -1, error('Δεν βρέθηκε το C_data.bin. Έτρεξες το πρόγραμμα της C?'); end
C = fread(fid, [d, N], 'double')'; 
fclose(fid);

% Διάβασμα του Q_data.bin (Ο πίνακας ερωτημάτων)
fid = fopen('Q_data.bin', 'r');
if fid == -1, error('Δεν βρέθηκε το Q_data.bin. Έτρεξες το πρόγραμμα της C?'); end
Q = fread(fid, [d, N], 'double')'; 
fclose(fid);

% Διάβασμα των Δεικτών (idx) που βρήκε η C
fid = fopen('idx_results.bin', 'r');
if fid == -1, error('Δεν βρέθηκε το idx_results.bin.'); end
idx_c = fread(fid, [k, N], 'int32')';
fclose(fid);

% Διάβασμα των Αποστάσεων (dst) που βρήκε η C
fid = fopen('dst_results.bin', 'r');
if fid == -1, error('Δεν βρέθηκε το dst_results.bin.'); end
dst_c = fread(fid, [k, N], 'double')';
fclose(fid);

fprintf('Τα δεδομένα φορτώθηκαν επιτυχώς!\n\n');

%% 3. ΕΚΤΕΛΕΣΗ ΤΗΣ ΣΥΝΑΡΤΗΣΗΣ ΤΟΥ MATLAB
fprintf('=== ΕΚΤΕΛΕΣΗ MATLAB KNNSEARCH ===\n');
tic;
% Η MATLAB χρησιμοποιεί κεφαλαίο 'K' στην παράμετρο συνήθως
[idx_m, dst_m] = knnsearch(C, Q, 'K', k); 
time_matlab = toc;
fprintf('Ο χρόνος εκτέλεσης του MATLAB ήταν: %.4f δευτερόλεπτα\n\n', time_matlab);

%% 4. ΣΥΓΚΡΙΣΗ ΚΑΙ ΕΠΑΛΗΘΕΥΣΗ (C vs MATLAB)
fprintf('=== ΣΥΓΚΡΙΣΗ ΑΠΟΤΕΛΕΣΜΑΤΩΝ ===\n');

% Α. Σύγκριση Αποστάσεων (Distances)
% Χρησιμοποιούμε όριο 1e-5 για να αγνοήσουμε τα floating-point errors
max_dist_error = max(abs(dst_c(:) - dst_m(:)));
fprintf('Μέγιστο σφάλμα στις αποστάσεις: %e\n', max_dist_error);

if max_dist_error < 1e-5
    fprintf('-> [PASS] Οι αποστάσεις είναι ΣΩΣΤΕΣ!\n');
else
    fprintf('-> [FAIL] Υπάρχει σημαντική διαφορά στις αποστάσεις.\n');
end

% Β. Σύγκριση Δεικτών (Indices)
matching_indices = sum(idx_c(:) == idx_m(:));
total_indices = numel(idx_c);
match_percentage = (matching_indices / total_indices) * 100;

fprintf('Συμφωνία στους δείκτες: %d / %d (%.2f%%)\n', matching_indices, total_indices, match_percentage);

if match_percentage == 100
    fprintf('-> [PASS] Οι δείκτες (γείτονες) ταυτίζονται 100%%!\n');
else
    fprintf('-> [WARNING] Οι δείκτες δεν ταυτίζονται 100%%.\n');
    fprintf('   (Σημείωση: Αν οι αποστάσεις είναι σωστές, αυτό μπορεί να συμβεί \n');
    fprintf('   αν 2 σημεία έχουν ΑΚΡΙΒΩΣ την ίδια απόσταση από το Q και η C \n');
    fprintf('   τα ταξινόμησε με διαφορετική σειρά από το MATLAB).\n');
end