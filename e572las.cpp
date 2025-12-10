// e572las.cpp : Defines the entry point for the console application.

#include <E57Simple.h>
#include <E57Foundation.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include <exception>
#include "lasreader.hpp"
#include "laswriter.hpp"
#undef min
#undef max

// we do not have an implementation for that
#undef COMPILE_WITH_MULTI_CORE
#undef COMPILE_WITH_GUI

#define HEADER_CHAR_LEN_MAX 4096

//#include "geoprojectionconverter.hpp"

class LASquaternion {
public:
  LASquaternion operator + (const LASquaternion& q) const
  {
    return LASquaternion(w + q.w, x + q.x, y + q.y, z + q.z);
  };
  LASquaternion operator - (const LASquaternion& q) const
  {
    return LASquaternion(w - q.w, x - q.x, y - q.y, z - q.z);
  };
  LASquaternion operator * (const LASquaternion& q) const
  {
    double w_val = w * q.w - x * q.x - y * q.y - z * q.z;
    double x_val = w * q.x + x * q.w + y * q.z - z * q.y;
    double y_val = w * q.y + y * q.w + z * q.x - x * q.z;
    double z_val = w * q.z + z * q.w + x * q.y - y * q.x;
    return LASquaternion(w_val, x_val, y_val, z_val);
  };
  LASquaternion operator / (LASquaternion& q) const
  {
    return ((*this) * (q.inverse()));
  };
  LASquaternion& operator += (const LASquaternion& q)
  {
    w += q.w;
    x += q.x;
    y += q.y;
    z += q.z;
    return (*this);
  };
  LASquaternion& operator -= (const LASquaternion& q)
  {
    w -= q.w;
    x -= q.x;
    y -= q.y;
    z -= q.z;
    return (*this);
  };
  LASquaternion& operator *= (const LASquaternion& q)
  {
    double w_val = w * q.w - x * q.x - y * q.y - z * q.z;
    double x_val = w * q.x + x * q.w + y * q.z - z * q.y;
    double y_val = w * q.y + y * q.w + z * q.x - x * q.z;
    double z_val = w * q.z + z * q.w + x * q.y - y * q.x;
    w = w_val;
    x = x_val;
    y = y_val;
    z = z_val;
    return (*this);
  };
  LASquaternion& operator /= (LASquaternion& q)
  {
    (*this) = (*this) * q.inverse();
    return (*this);
  };
  bool operator != (const LASquaternion& q) const
  {
    return ((w != q.w) || (x != q.x) || (y != q.y) || (z != q.z)) ? true : false;
  };
  bool operator == (const LASquaternion& q) const
  {
    return ((w == q.w) && (x == q.x) && (y == q.y) && (z == q.z)) ? true : false;
  };
  double norm() const
  {
    return (w * w + x * x + y * y + z * z);
  };
  double magnitude() const
  {
    return std::sqrt(norm());
  };
  LASquaternion conjugate() const
  {
    return LASquaternion(w, -x, -y, -z);
  };
  LASquaternion scale(double s) const
  {
    return LASquaternion(w * s, x * s, y * s, z * s);
  };
  LASquaternion inverse() const
  {
    return conjugate().scale(1 / norm());
  };
  void rotate(double& v0, double& v1, double& v2) const
  {
    LASquaternion qv(0, v0, v1, v2);
    LASquaternion qm = ((*this) * qv) * (*this).inverse();
    v0 = qm.x;
    v1 = qm.y;
    v2 = qm.z;
  };
  LASquaternion UnitQuaternion() const
  {
    return (*this).scale(1 / (*this).magnitude());
  };
  LASquaternion()
  {
    w = 1;
    x = y = z = 0;
  };
  LASquaternion(double w, double x, double y, double z)
  {
    this->w = w;
    this->x = x;
    this->y = y;
    this->z = z;
  };
  double w, x, y, z;
};

void usage(bool error = false, bool wait = false)
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "e572las -i in.e57 -o out.las\n");
  fprintf(stderr, "e572las -i in.e57 -o out.laz -split_scans\n");
  fprintf(stderr, "e572las -i in.e57 -o out.txt -oparse xyziRGB\n");
  fprintf(stderr, "e572las -i in.e57 -o out.las -set_scale 0.0001 0.0001 0.0001\n");
  fprintf(stderr, "e572las -i in.e57 -o out.txt -oparse xyzi -split_scans -include_invalid\n");
  fprintf(stderr, "e572las -i in.e57 -o out.laz -no_translation\n");
  fprintf(stderr, "e572las -i in.e57 -o out.laz -no_rotation\n");
  fprintf(stderr, "e572las -i in.e57 -o out.laz -no_pose\n");
  fprintf(stderr, "e572las -h\n");
  if (wait)
  {
    fprintf(stderr, "<press ENTER>\n");
    (void)getc(stdin);
  }
  exit(error);
}

int main(int argc, char* argv[])
{
  wait_on_exit(argc == 1);
  int i;
  bool verbose = false;
  bool very_verbose = false;
  char* file_name = 0;
  char* file_name_out = 0;
  bool merge_scans = true;
  bool apply_pose = true;
  bool apply_quaternion = true;
  LASquaternion quaternion;
  bool scan_has_quaternion = false;
  bool apply_translation = true;
  e57::Translation translation;
  bool scan_has_translation = false;
  bool include_invalid = false;
  double scale_factor[3] = { 0.001, 0.001, 0.001 };
  // int cores = 1;
  bool print_scan_count = false;
  std::vector<int> scan_vector;

  // Parse the command line

  LASwriter* laswriter = nullptr;
  //	LASreadOpener lasreadopener;
  LASwriteOpener laswriteopener;
  //	GeoProjectionConverter geoprojectionconverter;

  if (argc == 1)
  {
    fprintf(stderr, "e572las.exe is better run in the command line\n");
    char file_name_temp[256];
    fprintf(stderr, "enter input file: "); fgets(file_name_temp, 256, stdin);
    file_name_temp[strlen(file_name_temp) - 1] = '\0';
    //		lasreadopener.set_file_name(file_name_temp);
    file_name = LASCopyString(file_name_temp);
    fprintf(stderr, "enter output file: "); fgets(file_name_temp, 256, stdin);
    file_name_temp[strlen(file_name_temp) - 1] = '\0';
    laswriteopener.set_file_name(file_name_temp);
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == (char)(-106)) argv[i][0] = '-';
    }
    //		geoprojectionconverter.parse(argc, argv);
    //		lasreadopener.parse(argc, argv);
    laswriteopener.parse(argc, argv);
  }

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-hh") == 0 || strcmp(argv[i], "-help") == 0)
    {
      fprintf(stderr, "LAStools (by rapidlasso GmbH) version %d (%s)\n", LAS_TOOLS_VERSION, "freeware");
      usage();
    }
    else if (strcmp(argv[i], "-quiet") == 0)
    {
      set_message_log_level(LAS_QUIET);
    }
    else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-verbose") == 0)
    {
      verbose = true;
      set_message_log_level(LAS_VERBOSE);
    }
    else if (strcmp(argv[i], "-vv") == 0 || strcmp(argv[i], "-very_verbose") == 0)
    {
      verbose = true;
      very_verbose = true;
      set_message_log_level(LAS_VERY_VERBOSE);
    }
    else if (strcmp(argv[i], "-version") == 0)
    {
      fprintf(stderr, "LAStools (by rapidlasso GmbH) version %d (%s)\n", LAS_TOOLS_VERSION, "freeware");
      byebye();
    }
    else if (strcmp(argv[i], "-license") == 0)
    {
      fprintf(stderr, "LAStools (by rapidlasso GmbH) version %d (%s)\n", LAS_TOOLS_VERSION, "freeware");
      byebye();
    }
    else if (strcmp(argv[i], "-gui") == 0)
    {
#ifdef COMPILE_WITH_GUI
      gui = true;
#else
      fprintf(stderr, "WARNING: not compiled with GUI support. ignoring '-gui' ...\n");
#endif
    }
    else if (strcmp(argv[i], "-cores") == 0)
    {
#ifdef COMPILE_WITH_MULTI_CORE
      if ((i + 1) >= argc)
      {
        fprintf(stderr, "ERROR: '%s' needs 1 argument: number\n", argv[i]);
        byebye();
      }
      argv[i][0] = '\0';
      i++;
      cores = atoi(argv[i]);
      argv[i][0] = '\0';
#else
      fprintf(stderr, "WARNING: not compiled with multi-core batching. ignoring '-cores' ...\n");
#endif
    }
    else if (strcmp(argv[i], "-set_scale") == 0)
    {
      if ((i + 3) >= argc)
      {
        fprintf(stderr, "ERROR: '%s' needs 3 arguments: x_scale_factor y_scale_factor z_scale_factor\n", argv[i]);
        byebye();
      }
      argv[i][0] = '\0';
      i++;
      scale_factor[0] = atof(argv[i]);
      argv[i][0] = '\0';
      i++;
      scale_factor[1] = atof(argv[i]);
      argv[i][0] = '\0';
      i++;
      scale_factor[2] = atof(argv[i]);
      argv[i][0] = '\0';
    }
    else if ((strcmp(argv[i], "-split") == 0) || (strcmp(argv[i], "-split_scans") == 0))
    {
      merge_scans = false;
    }
    else if ((strcmp(argv[i], "-no_pose") == 0))
    {
      apply_translation = false;
      apply_quaternion = false;
    }
    else if ((strcmp(argv[i], "-no_translation") == 0))
    {
      apply_translation = false;
    }
    else if ((strcmp(argv[i], "-no_rotation") == 0))
    {
      apply_quaternion = false;
    }
    else if ((strcmp(argv[i], "-include_invalid") == 0))
    {
      include_invalid = true;
    }
    else if (strcmp(argv[i], "-i") == 0)
    {
      if ((i + 1) >= argc)
      {
        fprintf(stderr, "ERROR: '%s' needs 1 argument: file_name\n", argv[i]);
        byebye();
      }
      argv[i][0] = '\0';
      i++;
      file_name = LASCopyString(argv[i]);
      argv[i][0] = '\0';
    }
    else if (strcmp(argv[i], "-print_scan_count") == 0)
    {
      print_scan_count = true;
      set_message_log_level(LAS_QUIET);
    }
    else if (strcmp(argv[i], "-scan") == 0)
    {
      if ((i + 1) >= argc)
      {
        laserror("'%s' needs at least 1 argument: scan number [1..n]", argv[i]);
        return FALSE;
      }
      int i_in = i;
      i += 1;
      U32 scan;
      do
      {
        if ((sscanf(argv[i], "%u", &scan) != 1) || (scan <= 0))
        {
          laserror("'%s' needs at least 1 argument [1..n]. '%s' is not valid.", argv[i_in], argv[i]);
          return FALSE;
        }
        else
        {
          scan_vector.push_back(scan);
        }
        *argv[i] = '\0';
        i += 1;
      } while ((i < argc) && ('0' < *argv[i]) && (*argv[i] <= '9'));
      *argv[i_in] = '\0';
      i -= 1;
    }
    else if ((argv[i][0] != '-') && (file_name == 0))
    {
      // no filename given yet and unknown argument: take this as filename (allows "e572las foo.e57")
      file_name = LASCopyString(argv[i]);
      argv[i][0] = '\0';
    }
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      byebye();
    }
  }

  if (get_message_log_level() < LAS_QUIET)
  {
    fprintf(stderr, "===========================================================================\n");
    fprintf(stderr, "e572las.exe is a free tool by 'rapidlasso GmbH' for converting point clouds\n");
    fprintf(stderr, "from the E57 format to the LAS & LAZ format. Please also have a look at our\n");
    fprintf(stderr, "LAStools software at https://rapidlasso.de/LAStools if you like this tool.\n");
    fprintf(stderr, "===========================================================================\n");
  }

  if (file_name == 0)
  {
    fprintf(stderr, "ERROR: no input\n");
    byebye();
  }

  // Check if file exists

  FILE* file = LASfopen(file_name, "rb");
  if (file == 0)
  {
    fprintf(stderr, "ERROR: file '%s' does not exist\n", file_name);
    byebye();
  }
  fclose(file);

  try {

    // Instantiate an e57::Reader object and open the .e57 file.
#pragma warning(push)
#pragma warning(disable : 6387)
    e57::Reader eReader(file_name);
#pragma warning(pop)
    if (!eReader.IsOpen())
    {
      fprintf(stderr, "ERROR: opening '%s'\n", file_name);
      byebye();
    }

    // How many individual scans are there?

    int data3DCount = eReader.GetData3DCount();

    // option: just print scan count and exit
    if (print_scan_count) {
      fprintf(stdout, "%d\n", data3DCount);
      byebye();
    }

    LASMessage(LAS_VERBOSE, "file '%s' contains %d scan%s", file_name, data3DCount, (data3DCount == 1 ? "" : (merge_scans ? "s. merging ..." : "s. splitting ...")));

    // Loop over all scans
    LASheader header;
    LASpoint point;
    int64_t total_number_invalid_points = 0;
    int64_t total_number_points = 0;
    int scanIndex;
    int scanIndexFirst = 0;
    int scanIndexLast = data3DCount - 1;
    // just some scans requested: get first and last scan
    if (scan_vector.size() > 0)
    {
      scanIndexFirst = data3DCount - 1;
      scanIndexLast = 0;
      for (size_t loop = 0; loop < scan_vector.size(); loop++)
      {
        if (scan_vector[loop] > data3DCount)
        {
          LASMessage(LAS_WARNING, "scan number [%d] is bigger than number of scans %d in file and will be ignored", scan_vector[loop], data3DCount);
        }
        else
        {
          scanIndexFirst = std::min(scanIndexFirst, scan_vector[loop] - 1);
          scanIndexLast = std::max(scanIndexLast, scan_vector[loop] - 1);
        }
      }
      if (scanIndexLast - scanIndexFirst < 0)
      {
        laserror("given scan numbers does not match any available scans");
      }
    }

    for (scanIndex = scanIndexFirst; scanIndex < scanIndexLast + 1; scanIndex++)
    {
      // option: just do certain scans [1...n]
      if ((scan_vector.size() > 0) && std::find(scan_vector.begin(), scan_vector.end(), scanIndex + 1) == scan_vector.end())
      {
        continue;
      }

      // Read and access all the e57::Data3D header information from the first scan.
      e57::Data3D	scanHeader;
      eReader.ReadData3D(scanIndex, scanHeader);

      // check content of scan header
      bool spherical = false;
      if (scanHeader.pointFields.cartesianXField || scanHeader.pointFields.cartesianYField || scanHeader.pointFields.cartesianZField)
      {
        if (!scanHeader.pointFields.cartesianXField)
        {
          fprintf(stderr, "no cartesian x coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        if (!scanHeader.pointFields.cartesianYField)
        {
          fprintf(stderr, "no cartesian y coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        if (!scanHeader.pointFields.cartesianZField)
        {
          fprintf(stderr, "no cartesian z coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        spherical = false;
      }
      else if (scanHeader.pointFields.sphericalRangeField || scanHeader.pointFields.sphericalAzimuthField || scanHeader.pointFields.sphericalElevationField)
      {
        if (!scanHeader.pointFields.sphericalRangeField)
        {
          fprintf(stderr, "no spherical range coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        if (!scanHeader.pointFields.sphericalAzimuthField)
        {
          fprintf(stderr, "no spherical azimuth coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        if (!scanHeader.pointFields.sphericalElevationField)
        {
          fprintf(stderr, "no spherical elevation coordinates for scan %d. skipping ...\n", scanIndex);
          continue;
        }

        spherical = true;
      }
      else
      {
        fprintf(stderr, "neither cartesian nor coordinates for scan %d. skipping ...\n", scanIndex);
        continue;
      }

      // Get the size information about the scan.

      int64_t nColumn = 0;		// Number of Columns in a structure scan (from "indexBounds" if structure data)
      int64_t nRow = 0;			// Number of Rows in a structure scan	
      int64_t nPointsSize = 0;	// Number of points 
      int64_t nGroupsSize = 0;	// Number of groups (from "groupingByLine" if present)
      int64_t nCountsSize = 0;	// Number of points per group
      bool bColumnIndex = 0;

      eReader.GetData3DSizes(scanIndex, nRow, nColumn, nPointsSize, nGroupsSize, nCountsSize, bColumnIndex);

      LASMessageStream(LAS_VERBOSE) << "processing scan " << scanIndex + 1 << "\n";
      if (nRow && nColumn)
        LASMessageStream(LAS_VERBOSE) << "  contains grid of " << nColumn << " by " << nRow << " equaling " << nColumn * nRow << " " << (spherical ? "spherical" : "cartesian") << " points\n";
      else
        LASMessageStream(LAS_VERBOSE) << "  contains " << nPointsSize << " " << (spherical ? "spherical" : "cartesian") << " points\n";

      // Check if scan has pose information

      apply_pose = false;

      if ((scanHeader.pose.rotation.w != 1) || (scanHeader.pose.rotation.x != 0) || (scanHeader.pose.rotation.y != 0) || (scanHeader.pose.rotation.z != 0))
      {
        quaternion = LASquaternion(scanHeader.pose.rotation.w, scanHeader.pose.rotation.x, scanHeader.pose.rotation.y, scanHeader.pose.rotation.z);
        scan_has_quaternion = true;
        if (apply_quaternion) apply_pose = true;
        LASMessage(LAS_VERBOSE, "  has quaternion (%g,%g,%g,%g) which is %sapplied", quaternion.w, quaternion.x, quaternion.y, quaternion.z, (apply_quaternion ? "" : "not "));
      }
      else
      {
        scan_has_quaternion = false;
      }
      if ((scanHeader.pose.translation.x != 0) || (scanHeader.pose.translation.y != 0) || (scanHeader.pose.translation.z != 0))
      {
        translation = scanHeader.pose.translation;
        scan_has_translation = true;
        if (apply_translation) apply_pose = true;
        LASMessage(LAS_VERBOSE, "  has translation (%g,%g,%g) which is %sapplied", translation.x, translation.y, translation.z, (apply_translation ? "" : "not "));
      }
      else
      {
        scan_has_translation = false;
      }

      // Pick a size for the buffers

      int32_t nSize = (nRow > 0) ? (int32_t)nRow : 1024;

      // Setup the invalid data buffers if present

      int8_t* isInvalidData = NULL;

      if (spherical)
      {
        if (scanHeader.pointFields.sphericalInvalidStateField)
        {
          isInvalidData = new int8_t[nSize];
        }
      }
      else
      {
        if (scanHeader.pointFields.cartesianInvalidStateField)
        {
          isInvalidData = new int8_t[nSize];
        }
      }

      // Setup the intensity buffers if present

      double* intData = NULL;
      // bool bIntensity = false;
      double intRange = 0;
      double intOffset = 0;

      if (scanHeader.pointFields.intensityField)
      {
        // bIntensity = true;
        intData = new double[nSize];
        intRange = scanHeader.intensityLimits.intensityMaximum - scanHeader.intensityLimits.intensityMinimum;
        intOffset = scanHeader.intensityLimits.intensityMinimum;

        LASMessage(LAS_VERBOSE, "  contains intensities (%g-%g)", scanHeader.intensityLimits.intensityMinimum, scanHeader.intensityLimits.intensityMaximum);
      }

      // Setup color buffers if present

      uint16_t* redData = NULL;
      uint16_t* greenData = NULL;
      uint16_t* blueData = NULL;
      bool bColor = false;
      double colorRedRange = 1;
      double colorRedOffset = 0;
      double colorGreenRange = 1;
      double colorGreenOffset = 0;
      double colorBlueRange = 1;
      double colorBlueOffset = 0;

      if (scanHeader.pointFields.colorRedField && scanHeader.pointFields.colorGreenField && scanHeader.pointFields.colorBlueField)
      {
        bColor = true;
        redData = new uint16_t[nSize];
        greenData = new uint16_t[nSize];
        blueData = new uint16_t[nSize];
        colorRedRange = scanHeader.colorLimits.colorRedMaximum - scanHeader.colorLimits.colorRedMinimum;
        colorRedOffset = scanHeader.colorLimits.colorRedMinimum;
        colorGreenRange = scanHeader.colorLimits.colorGreenMaximum - scanHeader.colorLimits.colorGreenMinimum;
        colorGreenOffset = scanHeader.colorLimits.colorGreenMinimum;
        colorBlueRange = scanHeader.colorLimits.colorBlueMaximum - scanHeader.colorLimits.colorBlueMinimum;
        colorBlueOffset = scanHeader.colorLimits.colorBlueMinimum;

        LASMessage(LAS_VERBOSE, "  contains RGB colors (%g-%g, %g-%g, %g-%g)", (float)scanHeader.colorLimits.colorRedMinimum, (float)scanHeader.colorLimits.colorRedMaximum, (float)scanHeader.colorLimits.colorGreenMinimum, (float)scanHeader.colorLimits.colorGreenMaximum, (float)scanHeader.colorLimits.colorBlueMinimum, (float)scanHeader.colorLimits.colorBlueMaximum);
      }

      // Setup the GroupByLine buffers information if present

      int64_t* idElementValue = NULL;
      int64_t* startPointIndex = NULL;
      int64_t* pointCount = NULL;

      /* not used right now
          if(nGroupsSize > 0)
          {
            idElementValue = new int64_t[(uint32_t)nGroupsSize];
            startPointIndex = new int64_t[(uint32_t)nGroupsSize];
            pointCount = new int64_t[(uint32_t)nGroupsSize];

            if (!eReader.ReadData3DGroupsData(scanIndex, (int32_t)nGroupsSize, idElementValue, startPointIndex, pointCount))
            {
              nGroupsSize = 0;
            }
          }
      */

      // Setup the row/column index information if present

      int32_t* rowIndex = NULL;
      int32_t* columnIndex = NULL;

      /* not used right now
          if (scanHeader.pointFields.rowIndexField)
          {
              rowIndex = new int32_t[nSize];
          }
          if (scanHeader.pointFields.columnIndexField)
          {
              columnIndex = new int32_t[(uint32_t)nRow];
          }
      */

      // Setup the return information if present

      int8_t* returnIndex = NULL;
      int8_t* returnCount = NULL;

      if (scanHeader.pointFields.returnIndexField)
      {
        returnIndex = new int8_t[nSize];
        LASMessage(LAS_VERBOSE, "  contains return indices");
      }
      if (scanHeader.pointFields.returnCountField)
      {
        returnCount = new int8_t[nSize];
        LASMessage(LAS_VERBOSE, "  contains return counts");
      }

      // Setup the time stamp information if present

      double* timeStamp = NULL;
      // int8_t* isTimeStampInvalid = NULL;

      if (scanHeader.pointFields.timeStampField)
      {
        timeStamp = new double[nSize];
        LASMessage(LAS_VERBOSE, "  contains time stamps");
      }
      /*
      if (scanHeader.pointFields.isTimeStampInvalidField)
      {
        isTimeStampInvalid = new int8_t[nSize];
      }
      */

      // Setup the xyz buffers and setup the e57::CompressedVectorReader point data object.

      double* cartesianX = (spherical ? NULL : new double[nSize]);
      double* cartesianY = (spherical ? NULL : new double[nSize]);
      double* cartesianZ = (spherical ? NULL : new double[nSize]);

      double* sphericalRange = (spherical ? new double[nSize] : NULL);
      double* sphericalAzimuth = (spherical ? new double[nSize] : NULL);
      double* sphericalElevation = (spherical ? new double[nSize] : NULL);

      // Setup the Compressed Vector Reader

      e57::CompressedVectorReader dataReader = eReader.SetUpData3DPointsData(
        scanIndex,                      //!< data block index given by the NewData3D
        nSize,                          //!< size of each of the buffers given
        cartesianX,                     //!< pointer to a buffer with the x coordinate (in meters) of the point in Cartesian coordinates 
        cartesianY,                     //!< pointer to a buffer with the y coordinate (in meters) of the point in Cartesian coordinates
        cartesianZ,                     //!< pointer to a buffer with the z coordinate (in meters) of the point in Cartesian coordinates
        isInvalidData,					//!< pointer to a buffer with the valid indication. Value = 0 if the point is considered valid, 1 otherwise
        intData,                        //!< pointer to a buffer with the lidar return intesity
        NULL,
        redData,                        //!< pointer to a buffer with the color red data. Unit is unspecified
        greenData,                      //!< pointer to a buffer with the color green data. Unit is unspecified
        blueData,                       //!< pointer to a buffer with the color blue data. Unit is unspecified
        NULL,
        sphericalRange,                 //!< pointer to a buffer with the range (in meters) of points in spherical coordinates. Shall be non-negative
        sphericalAzimuth,               //!< pointer to a buffer with the Azimuth angle (in radians) of point in spherical coordinates
        sphericalElevation,             //!< pointer to a buffer with the Elevation angle (in radians) of point in spherical coordinates
        isInvalidData,                  //!< pointer to a buffer with the valid indication. Value = 0 if the point is considered valid, 1 otherwise
        rowIndex,                       //!< pointer to a buffer with the rowIndex
        columnIndex,                    //!< pointer to a buffer with the columnIndex
        returnIndex,					//!< pointer to a buffer with the return index
        returnCount);					//!< pointer to a buffer with the return count

  // Create the file name (if needed)

      bool new_file = false;

      if (merge_scans)
      {
        if (scanIndex == scanIndexFirst)
        {
          if (!laswriteopener.get_file_name())
          {
            laswriteopener.make_file_name(file_name, -2);
          }
          new_file = true;
        }
      }
      else
      {
        if (scanIndex == scanIndexFirst)
        {
          char* file_name_temp;
          if (laswriteopener.get_file_name())
          {
            file_name_temp = LASCopyString(laswriteopener.get_file_name());
          }
          else
          {
            file_name_temp = LASCopyString(file_name);
          }
          int len = strlen(file_name_temp);
          file_name_out = (char*)malloc(len + 10);
          memset(file_name_out, 0, len + 10);
          strcpy(file_name_out, file_name_temp);
          while (len > 0 && file_name_temp[len] != '.')
          {
            file_name_out[len + 5] = file_name_temp[len];
            len--;
          }
          free(file_name_temp);
          file_name_out[len + 5] = '.';
          file_name_out[len + 4] = '0';
          file_name_out[len + 3] = '0';
          file_name_out[len + 2] = '0';
          file_name_out[len + 1] = '0';
          file_name_out[len + 0] = '0';
        }
        laswriteopener.set_force(TRUE);
        laswriteopener.make_file_name(file_name_out, scanIndex);
        new_file = true;
      }

      // Populate the LAS header

      if (new_file)
      {
        header.clean();

        // info about me

        strncpy_las(header.system_identifier, LAS_HEADER_CHAR_LEN, LAS_TOOLS_COPYRIGHT);
        sprintf(header.generating_software, "e572las.exe (version %d)", LAS_TOOLS_VERSION);

        // what date was the data created

        int year;
        int month;
        int day;
        int hour;
        int minute;
        float seconds;

        scanHeader.acquisitionStart.GetUTCDateTime(year, month, day, hour, minute, seconds);

        int startday[13] = { -1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
        header.file_creation_day = startday[month] + day;
        if (((year % 4) == 0) && (month > 2)) header.file_creation_day++; // leap year handling
        header.file_creation_year = year;

        // any other information in the header

        auto vlr_add = [&](std::string user_id, std::string str) -> void {
          U32 length;
          U8* data;
          length = (U32)str.length();
          if (length) {
            length = (length < HEADER_CHAR_LEN_MAX ? length : HEADER_CHAR_LEN_MAX);
            data = new U8[length + 1];
            strncpy_las((CHAR*)data, length + 1, str.c_str(), length);
            header.add_vlr(user_id.c_str(), 4711, length, data, TRUE, (CHAR*)data);
          }
        };

        vlr_add("name", scanHeader.name);
        vlr_add("guid", scanHeader.guid);
        vlr_add("description", scanHeader.description);
        vlr_add("sensorVendor", scanHeader.sensorVendor);
        vlr_add("sensorModel", scanHeader.sensorModel);
        vlr_add("sensorSerialNo", scanHeader.sensorSerialNumber);
        vlr_add("sensorHwVersion", scanHeader.sensorHardwareVersion);
        vlr_add("sensorSwVersion", scanHeader.sensorSoftwareVersion);
        vlr_add("sensorFwVersion", scanHeader.sensorFirmwareVersion);

        if (timeStamp)
        {
          header.point_data_format += 1;
          header.point_data_record_length += 8;
        }

        if (bColor)
        {
          header.point_data_format += 2;
          header.point_data_record_length += 6;
        }

        header.x_scale_factor = scale_factor[0];
        header.y_scale_factor = scale_factor[1];
        header.z_scale_factor = scale_factor[2];

        if (scan_has_translation && apply_translation)
        {
          header.x_offset = ((int)(translation.x / 10000)) * 10000;
          header.y_offset = ((int)(translation.y / 10000)) * 10000;
          header.z_offset = ((int)(translation.z / 10000)) * 10000;
        }
        else
        {
          if ((scanHeader.cartesianBounds.xMinimum == -DBL_MAX) || (scanHeader.cartesianBounds.xMaximum == DBL_MAX))
          {
            header.x_offset = 0.0;
          }
          else
          {
            header.x_offset = ((int)((scanHeader.cartesianBounds.xMinimum + scanHeader.cartesianBounds.xMaximum) / 20000)) * 10000;
          }
          if ((scanHeader.cartesianBounds.yMinimum == -DBL_MAX) || (scanHeader.cartesianBounds.yMaximum == DBL_MAX))
          {
            header.y_offset = 0.0;
          }
          else
          {
            header.y_offset = ((int)((scanHeader.cartesianBounds.yMinimum + scanHeader.cartesianBounds.yMaximum) / 20000)) * 10000;
          }
          if ((scanHeader.cartesianBounds.zMinimum == -DBL_MAX) || (scanHeader.cartesianBounds.zMaximum == DBL_MAX))
          {
            header.z_offset = 0.0;
          }
          else
          {
            header.z_offset = ((int)((scanHeader.cartesianBounds.zMinimum + scanHeader.cartesianBounds.zMaximum) / 20000)) * 10000;
          }
        }

        if (very_verbose)
        {
          if (spherical)
          {
            fprintf(stderr, "  range min %g, max %g, offset %g\n", scanHeader.sphericalBounds.rangeMinimum, scanHeader.sphericalBounds.rangeMaximum, header.x_offset);
            fprintf(stderr, "  elevation min %g, max %g, offset %g\n", scanHeader.sphericalBounds.elevationMinimum, scanHeader.sphericalBounds.elevationMaximum, header.y_offset);
            fprintf(stderr, "  azimuth min %g, zmax %g, offset %g\n", scanHeader.sphericalBounds.azimuthStart, scanHeader.sphericalBounds.azimuthEnd, header.z_offset);
          }
          else
          {
            fprintf(stderr, "  x min %g, max %g, offset %g\n", scanHeader.cartesianBounds.xMinimum, scanHeader.cartesianBounds.xMaximum, header.x_offset);
            fprintf(stderr, "  y min %g, max %g, offset %g\n", scanHeader.cartesianBounds.yMinimum, scanHeader.cartesianBounds.yMaximum, header.y_offset);
            fprintf(stderr, "  z min %g, max %g, offset %g\n", scanHeader.cartesianBounds.zMinimum, scanHeader.cartesianBounds.zMaximum, header.z_offset);
          }
        }

        if (verbose)
        {
          if ((header.x_scale_factor == 0.001) && (header.y_scale_factor == 0.001) && (header.z_scale_factor == 0.001))
          {
            fprintf(stderr, "  %s written with millimeter resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else if ((header.x_scale_factor == 0.01) && (header.y_scale_factor == 0.01) && (header.z_scale_factor == 0.01))
          {
            fprintf(stderr, "  %s written with centimeter resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else if ((header.x_scale_factor == 0.1) && (header.y_scale_factor == 0.1) && (header.z_scale_factor == 0.1))
          {
            fprintf(stderr, "  %s written with decimeter resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else if ((header.x_scale_factor == 0.0001) && (header.y_scale_factor == 0.0001) && (header.z_scale_factor == 0.0001))
          {
            fprintf(stderr, "  %s written with 0.1 mm resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else if ((header.x_scale_factor == 0.00001) && (header.y_scale_factor == 0.00001) && (header.z_scale_factor == 0.00001))
          {
            fprintf(stderr, "  %s written with 0.01 mm resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else if ((header.x_scale_factor == 0.000001) && (header.y_scale_factor == 0.000001) && (header.z_scale_factor == 0.000001))
          {
            fprintf(stderr, "  %s written with 0.001 mm resolution to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), laswriteopener.get_file_name());
          }
          else
          {
            fprintf(stderr, "  %s written with resolution %g %g %g to '%s'\n", (merge_scans && (data3DCount > 1) ? "all scans are" : "is"), header.x_scale_factor, header.y_scale_factor, header.z_scale_factor, laswriteopener.get_file_name());
          }
        }

        // Initialize the LAS point

        point.init(&header, header.point_data_format, header.point_data_record_length, &header);

        // Open the writer

        laswriter = laswriteopener.open(&header);

        if (laswriter == 0)
        {
          fprintf(stderr, "ERROR: opening '%s'", laswriteopener.get_file_name());
          byebye();
        }
      }

      // Set point source ID

      point.set_point_source_ID((U16)(scanIndex + 1));

      // Read the data into the buffers and write to LAS/LAZ/BIN/SHP/PLY

      unsigned int size;
      unsigned int number_invalid_points = 0;
      double x = 0;
      double y = 0;
      double z = 0;

      while ((size = dataReader.read()) > 0)
      {
        for (unsigned int i = 0; i < size; i++)
        {
          if (isInvalidData && isInvalidData[i])
          {
            number_invalid_points++;
            if (!include_invalid) continue;
          }

          total_number_points++;

          if (spherical)
          {
            double sinElevation = sin(sphericalElevation[i]);
            double cosElevation = cos(sphericalElevation[i]);
            double sinAzimuth = sin(sphericalAzimuth[i]);
            double cosAzimuth = cos(sphericalAzimuth[i]);
            x = sphericalRange[i] * cosElevation * cosAzimuth;
            y = sphericalRange[i] * cosElevation * sinAzimuth;
            z = sphericalRange[i] * sinElevation;
          }
          else
          {
            x = cartesianX[i];
            y = cartesianY[i];
            z = cartesianZ[i];
          }

          if (apply_pose)
          {
            if (scan_has_quaternion && apply_quaternion)
            {
              quaternion.rotate(x, y, z);
            }

            if (scan_has_translation && apply_translation)
            {
              x += translation.x;
              y += translation.y;
              z += translation.z;
            }
          }

          point.set_x(x);
          point.set_y(y);
          point.set_z(z);

          if (intData)
          {
            if ((intRange == 255) || (intRange == 65535))
            {
              point.intensity = (int)(intData[i] - intOffset);
            }
            else
            {
              point.intensity = ((int)(0.5 + (((intData[i] - intOffset) * 255) / intRange))) << 8;
            }
          }

          if (bColor)
          {
            point.rgb[0] = ((int)(0.5 + (((redData[i] - colorRedOffset) * 255) / colorRedRange))) << 8;
            point.rgb[1] = ((int)(0.5 + (((greenData[i] - colorGreenOffset) * 255) / colorGreenRange))) << 8;
            point.rgb[2] = ((int)(0.5 + (((blueData[i] - colorBlueOffset) * 255) / colorBlueRange))) << 8;
          }

          if (returnIndex)
          {
            point.return_number = (returnIndex[i] + 1) & 7;
          }

          if (returnCount)
          {
            point.number_of_returns = (returnCount[i] + 1) & 7;
          }

          if (timeStamp)
          {
            point.gps_time = timeStamp[i];
          }

          laswriter->write_point(&point);
          laswriter->update_inventory(&point);
        }
      }

      if (!merge_scans || (scanIndex == scanIndexLast))
      {
        laswriter->update_header(&header, TRUE);
        laswriter->close();
        delete laswriter;
        laswriter = 0;
        laswriteopener.set_file_name(0);
      }

      if (number_invalid_points)
      {
        LASMessage(LAS_VERBOSE, "  %u invalid points were %s", number_invalid_points, (include_invalid ? "included" : "omitted"));
        total_number_invalid_points += number_invalid_points;
      }

      // Close the reader and clean up the buffer.

      dataReader.close();

      if (spherical)
      {
        delete[] sphericalRange;
        delete[] sphericalElevation;
        delete[] sphericalAzimuth;
      }
      else
      {
        delete[] cartesianX;
        delete[] cartesianY;
        delete[] cartesianZ;
      }

      if (isInvalidData) delete[] isInvalidData;

      if (intData) delete[] intData;

      if (redData) delete[] redData;
      if (greenData) delete[] greenData;
      if (blueData) delete[] blueData;

      if (idElementValue) delete[] idElementValue;
      if (startPointIndex) delete[] startPointIndex;
      if (pointCount) delete[] pointCount;

      if (rowIndex) delete[] rowIndex;
      if (columnIndex) delete[] columnIndex;

      if (returnIndex) delete[] returnIndex;
      if (returnCount) delete[] returnCount;
    } // for

    if (total_number_invalid_points)
    {
      LASMessage(LAS_VERBOSE, "scan%s of '%s' contain%s %lld invalid points that were %s", (data3DCount > 1 ? "s" : ""), file_name, (data3DCount == 1 ? "s" : ""), total_number_invalid_points, (include_invalid ? "included" : "omitted"));
    }

    LASMessage(LAS_VERBOSE, "written a total %lld points", total_number_points);

  }
  catch (std::exception& e) {
    fprintf(stderr, "ERROR: processing '%s': %s", file_name, e.what());
    return 1; 
  };

  if (file_name) free(file_name);
  if (file_name_out) free(file_name_out);

  return 0;
}
