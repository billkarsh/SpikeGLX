#ifndef CsvFile_h_
#define CsvFile_h_

#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <stdint.h>
#include <cstdint>

/**
 * @class CsvFile
 *
 * This class represents a Comma Separated Values (CSV) file
 * It provides file read functions, and presents the data to the user as a
 * 2-dimensional array.
 */
class CsvFile
{
public:
  /**
   * @brief Constructor, file is default closed, filename set to given value.
   *
   * @param filename: the name of the csv file
   */
  CsvFile(const std::string filename = "");
  ~CsvFile();

  /**
   * @brief Check whether the element at the given row and column exists.
   *
   * @param row: the row of the element
   * @param column: the column of the element
   *
   * @return true if the element exists, false otherwise
   */
  bool elementExists(unsigned int row, unsigned int column) const;

  /**
   * @brief Check whether the element at the given row and column is empty.
   *
   * @param row: the row of the element
   * @param column: the column of the element
   *
   * @return true if the element is empty, false otherwise
   */
  bool isEmpty(unsigned int row, unsigned int column) const;

  /**
   * @brief Get the element at the given row and column.
   *
   * This function returns the element of the given row and column of the table
   * of this csv file. If the element at this position is not available, an
   * empty string is returned.
   *
   * @param row: the row of the desired element
   * @param column: the column of the desired element
   *
   * @return the element at the given position
   */
  std::string getElement(unsigned int row, unsigned int column) const;

  /**
   * @brief Get the element at the given row and column as an unsigned int.
   *
   * @param row: the row of the desired element
   * @param column: the column of the desired element
   *
   * @return the element at the given position, as an unsigned int
   *
   * @throws std::runtime_error if the element could not be converted
   */
  unsigned int getUint(unsigned int row, unsigned int column) const;

  /**
   * @brief Get the element at the given row and column as an uint64_t.
   *
   * @param row: the row of the desired element
   * @param column: the column of the desired element
   *
   * @return the element at the given position, as an uint64_t
   *
   * @throws std::runtime_error if the element could not be converted
   */
  uint64_t getUint64(unsigned int row, unsigned int column) const;

  /**
   * @brief Get the element at the given row and column as a float.
   *
   * @param row: the row of the desired element
   * @param column: the column of the desired element
   *
   * @return the elemenet at the given position, as a float
   *
   * @throws std::runtime_error if the element could not be converted
   */
  float getFloat(unsigned int row, unsigned int column) const;

  /**
   * @brief Get the number of rows in the table.
   *
   * @return the number of rows
   */
  unsigned int getNumberOfRows() const;

  /**
   * @brief Read the csv file
   *
   * This function opens the file with the filename of this csv file, detects
   * the delimiter used in that file, reads the complete file into the table of
   * this csv file, and closes the file again.
   *
   * @throws std::runtime_error if the file could not be read
   */
  void read();

private:
  std::string fileName_;
  std::vector<std::vector<std::string> > table_;
};

#endif
