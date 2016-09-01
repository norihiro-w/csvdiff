/* Copyright 2016 Norihiro Watanabe. All rights reserved.

csvdiff is free software; you can redistribute and use in source and 
binary forms, with or without modification. tec2vtk is distributed in the 
hope that it will be useful but WITHOUT ANY WARRANTY; without even the 
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <valarray>
#include <vector>

#include <tclap/CmdLine.h>

void trim(std::string &str, char ch = ' ')
{
    std::string::size_type pos = str.find_last_not_of(ch);
    if(pos != std::string::npos)
    {
        str.erase(pos + 1);
        pos = str.find_first_not_of(ch);
        if(pos != std::string::npos)
            str.erase(0, pos);
    }
    else
        str.erase(str.begin(), str.end());
}

std::vector<std::string> split(const std::string &str, char delim)
{
    std::vector<std::string> strList;
    std::stringstream ss(str);
    std::string item;
    while(getline(ss, item, delim))
    {
        trim(item, ' ');
        trim(item, '\"');
        strList.push_back(item);
    }
    return strList;
}

template<typename T> T str2number (const std::string &str)
{
    std::stringstream ss(str);
    T v;
    ss >> v;
    return v;
}

template<typename T>
std::vector<T> str2number(const std::vector<std::string> &vec_str)
{
    std::vector<T> vec_num;
    vec_num.reserve(vec_str.size());
    for (auto str : vec_str)
    {
        vec_num.push_back(str2number<T>(str));
    }
    return vec_num;
}

unsigned getNLines(std::ifstream &is)
{
    std::string line;
    unsigned n_lines = 0;
    while (std::getline(is, line))
    {
        trim(line);
        if (!line.empty())
            ++n_lines;
    }

    is.clear();
    is.seekg(0, std::ios_base::beg);

    return n_lines;
}

struct CSVData
{
    std::vector<std::string> column_names;
    unsigned n_columns = 0;
    unsigned n_rows = 0;
    std::vector<double> matrix;
};


std::unique_ptr<CSVData> readCSV(std::string const& filepath, char delim)
{
    //std::cout << "reading " << filepath << std::endl;
    std::ifstream is(filepath);
    if (!is) {
        std::cout << "Failed to open " << filepath << std::endl;
        return nullptr;
    }
    const unsigned n_lines = getNLines(is);
    if (n_lines < 2)
    {
        std::cout << "The file is empty or cotains only one line." << filepath << std::endl;
        return nullptr;
    }

    std::unique_ptr<CSVData> csv(new CSVData());

    std::string line;
    unsigned current_line = 0;

    // header
    std::getline(is, line);
    current_line++;
    csv->column_names = split(line, delim);
    if (csv->column_names.empty())
    {
        std::cout << "No column name found." << filepath << std::endl;
        return nullptr;
    }

    // set matrix
    csv->n_columns = csv->column_names.size();
    csv->n_rows = n_lines - 1; // exclude the header
    csv->matrix.resize(csv->n_rows * csv->n_columns);

    // values
    unsigned rows = 0;
    while (std::getline(is, line))
    {
        trim(line);
        if (line.empty())
            continue;
        current_line++;
        auto const values_str = split(line, delim);
        if (csv->n_columns != values_str.size())
        {
            std::cout << "Line " << current_line << ": the number of columns (" << values_str.size() << ") is different from that of the column header (" << csv->n_columns << ")" << std::endl;
            return nullptr;
        }
        auto const values_num(str2number<double>(values_str));
        for (unsigned j=0; j<values_num.size(); j++)
            csv->matrix[rows * csv->n_columns + j] = values_num[j];
        rows++;
    }

    return csv;
}

bool diffCSV(CSVData const& csvA, CSVData const& csvB, std::vector<double> const& abs_tol, std::vector<double> const& rel_tol, bool verbose)
{
    // diff general
    if (csvA.n_columns != csvB.n_columns)
    {
        std::cout << "The number of columns are different: " << csvA.n_columns << " vs " << csvB.n_columns << std::endl;
        return false;
    }
    if (csvA.n_rows != csvB.n_rows)
    {
        std::cout << "The number of rows are different: " << csvA.n_rows << " vs " << csvB.n_rows << std::endl;
        return false;
    }

    // get the max abs value for each column
    std::vector<double> col_max_abs(csvA.n_columns, 0);
    for (unsigned i=0; i<csvA.n_rows; i++)
    {
        for (unsigned j=0; j<csvA.n_columns; j++)
        {
            col_max_abs[j] = std::max(col_max_abs[j], std::abs(csvA.matrix[i*csvA.n_columns + j]));
        }
    }

    // look for vector values
    std::set<std::string> vector_names;
    for (std::string const& name : csvA.column_names)
    {
        auto pos = name.find("_");
        if (pos == std::string::npos)
            continue;

        vector_names.insert(name.substr(0, pos));
    }

    // set the same max value to all the vector components
    for (std::string const& vector_name : vector_names)
    {
        double max_abs = 0;
        std::vector<size_t> comp_ids;
        for (size_t i=0; i<csvA.column_names.size(); i++)
        {
            if (csvA.column_names[i].find(vector_name) == std::string::npos)
                continue;
            max_abs = std::max(max_abs, col_max_abs[i]);
            comp_ids.push_back(i);
        }
        for (auto i : comp_ids)
            col_max_abs[i] = max_abs;
    }

    // compare row by row, column by column
    unsigned n_diff_lines = 0;
    std::valarray<double> col_max_abs_err(0.0, csvA.n_columns);
    std::valarray<double> col_max_rel_err(0.0, csvA.n_columns);
    for (unsigned i=0; i<csvA.n_rows; i++)
    {
        bool isLineDiff = false;
        for (unsigned j=0; j<csvA.n_columns; j++)
        {
            double v1 = csvA.matrix[i*csvA.n_columns + j];
            double v2 = csvB.matrix[i*csvA.n_columns + j];
            auto abs_err = std::abs(v1 -v2);
            auto rel_err = col_max_abs[j]==0 ? 0 : abs_err/col_max_abs[j];
            col_max_abs_err[j] = std::max(col_max_abs_err[j], abs_err);
            col_max_rel_err[j] = std::max(col_max_rel_err[j], rel_err);

            if (abs_err <= abs_tol[j] || rel_err <= rel_tol[j])
                continue;
            isLineDiff = true;

            if (!verbose)
                continue;
            std::cout << "----------------\n";
            std::cout << "##" << (i+1) << " #:" << j << "  <== " << v1 << "\n";
            std::cout << "##" << (i+1) << " #:" << j << "  ==> " << v2 << "\n";
            std::cout << "@ Absolute error = " << abs_err << ", Relative error = " << rel_err  << "\n";
        }
        if (isLineDiff)
            n_diff_lines++;
    }

    if (n_diff_lines == 0)
        return true;

    std::cout << "\n";
    std::cout << "Found " << n_diff_lines << " lines contain different values beyond the tolerance\n";
    std::cout << "\n";
    for (unsigned j=0; j<csvA.n_columns; j++)
    {
        if (col_max_abs_err[j] <= abs_tol[j] || col_max_rel_err[j] <= rel_tol[j])
            continue;

        std::cout << csvA.column_names[j] << ":\n";
        std::cout << "-> max. abs. error = " << col_max_abs_err[j] << " (tol. = " << abs_tol[j] << ")\n";
        std::cout << "-> max. rel. error = " << col_max_rel_err[j] << " (tol. = " << rel_tol[j] << ")\n";
    }
    std::cout << "\n";
    std::cout << std::endl;

    return false;
}


int main(int argc, char* argv[])
{
    TCLAP::CmdLine cmd("csvdiff software", ' ', "0.1");

    TCLAP::UnlabeledValueArg<std::string> argFileNameA("input-file-a", "Path to a csv file.", true, "", "file path");
    cmd.add(argFileNameA);

    TCLAP::UnlabeledValueArg<std::string> argFileNameB("input-file-b", "Path to a csv file.", true, "", "file path");
    cmd.add(argFileNameB);

    TCLAP::SwitchArg argVerbose("v", "verbose", "print which values differ.", true);
    cmd.add(argVerbose);

    TCLAP::ValueArg<double> argAbsTol("a", "abs", "absolute tolerance", false, 1e-10, "float");
    cmd.add(argAbsTol);

    TCLAP::ValueArg<double> argRelTol("r", "rel", "relative tolerance", false, 1e-5, "float");
    cmd.add(argRelTol);

    cmd.parse(argc, argv);

    auto const filePathA = argFileNameA.getValue();
    auto const filePathB = argFileNameB.getValue();
    auto const abs_tol = argAbsTol.getValue();
    auto const rel_tol = argRelTol.getValue();
    auto const verbose = argVerbose.getValue();
    char const delim = ',';

    // read the CSV data
    std::unique_ptr<CSVData> csvA(readCSV(filePathA, delim));
    if (!csvA)
        return EXIT_FAILURE;
    std::unique_ptr<CSVData> csvB(readCSV(filePathB, delim));
    if (!csvB)
        return EXIT_FAILURE;

    // diff the CSV data
    std::vector<double> vec_abs_tol(csvA->n_columns, abs_tol);
    std::vector<double> vec_rel_tol(vec_abs_tol.size(), rel_tol);
    std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
    std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
    bool same = diffCSV(*csvA, *csvB, vec_abs_tol, vec_rel_tol, verbose);
    if (!same)
    {
        std::cout << "Found a difference between the two files\n";
        std::cout << "* " << filePathA << "\n";
        std::cout << "* " << filePathB << "\n";
        std::cout << "\n" << std::endl;
    }

    return same ? EXIT_SUCCESS : EXIT_FAILURE;
}
