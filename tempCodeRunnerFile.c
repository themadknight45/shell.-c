
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
        history[h_index]=line;
        h_index++;
		commandHandler(tokens);
	}          

	exit(0);
}