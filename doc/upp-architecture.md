# Comprehensive Project Plan for the Modernization of the NOAA Unified Post Processor (UPP)

## Introduction to the Modernization Imperative

The National Oceanic and Atmospheric Administration (NOAA) Unified Post Processor (UPP) stands as a foundational pillar within the computational infrastructure of the United States numerical weather prediction (NWP) enterprise. Historically developed and maintained by the National Centers for Environmental Prediction (NCEP), the UPP serves as the critical translation layer between the raw, complex outputs of atmospheric dynamical cores and the standardized meteorological products required by operational forecasters, automated alerting systems, and the broader scientific community. The software is explicitly designed to ingest raw prognostic data—such as variables defined on the staggered, hybrid sigma-pressure coordinate system of the Finite-Volume Cubed-Sphere (FV3) dynamical core—and process these into hundreds of meteorologically significant diagnostic products. These products include fields interpolated onto isobaric and height surfaces, severe weather diagnostics such as Convective Available Potential Energy (CAPE) and vorticity, aviation products, radar reflectivities, and simulated satellite brightness temperatures derived via the Community Radiative Transfer Model (CRTM).

As NOAA and the Earth Prediction Innovation Center (EPIC) drive the meteorological community toward the Unified Forecast System (UFS), a massive consolidation effort is underway to replace dozens of legacy standalone modeling systems with a single, coupled Earth modeling framework. The UPP is already heavily integrated into this ecosystem, serving as the primary post-processing engine for the Global Forecast System (GFS), the Rapid Refresh Forecast System (RRFS), the Hurricane Analysis and Forecast System (HAFS), and both the Medium-Range Weather (MRW) and Short-Range Weather (SRW) Applications. However, the legacy architecture of the UPP, which is deeply rooted in procedural Fortran routines housed within legacy repositories like `ncep_post.fd`, is increasingly strained by the demands of exascale computing, heterogeneous hardware architectures, and cloud-native operational environments.

To ensure that the UFS can operate seamlessly and efficiently across both on-premises High-Performance Computing (HPC) clusters and scalable commercial cloud environments, a comprehensive modernization of the UPP is mandatory. This report outlines an exhaustive, expert-level project plan for this modernization effort. The architectural blueprint relies on the `bbakernoaa/helm-project` as the foundational C++ backbone, serving to orchestrate the post-processing lifecycle and decouple scientific logic from underlying data management. To guarantee performance and memory portability across diverse hardware (e.g., multi-core CPUs and modern GPUs), the plan integrates the Kokkos programming model alongside the C++23 `mdspan` library, bridging the persistent memory layout gap between legacy Fortran arrays and modern C++ data structures. Furthermore, the integration of an Asynchronous I/O (AMIO) framework will revolutionize data output, supporting legacy WMO-standard GRIB2 formats alongside NetCDF and cloud-optimized Zarr stores. Finally, the modernized UPP is architected to support dual execution modalities: operating dynamically as an inline library within the UFS weather model to eliminate disk I/O latency, and functioning independently as an offline C++ driver for historical reanalysis and cloud-centric workflows.

## The Legacy Constraints and the UFS Paradigm

Understanding the necessity of this modernization requires a rigorous examination of the limitations inherent in the legacy UPP architecture. For decades, numerical weather prediction systems were developed as monolithic Fortran applications tailored for homogeneous CPU-based supercomputers. The UPP codebase reflects this lineage. The core logic resides in directories such as `ncep_post.fd`, characterized by massive procedural files (e.g., `INITPOST.F`, `CALWXT_BOURG.f`, `SET_LVLSXML.f`) that tightly couple memory allocation, Message Passing Interface (MPI) communication, and scientific algorithms.

### The Exascale and Heterogeneous Hardware Challenge

The primary technical constraint of the legacy Fortran architecture is its inability to efficiently map to heterogeneous hardware architectures, specifically Graphics Processing Units (GPUs). As NOAA transitions to next-generation HPC systems to resolve atmospheric physics at sub-3-kilometer resolutions (as required by the HRRR and RRFS), the computational burden shifts from traditional CPU cores to massive arrays of GPU accelerators. The legacy UPP relies on standard nested Fortran `do` loops and basic OpenMP pragmas, which do not translate effectively to the Single Instruction, Multiple Threads (SIMT) execution models of NVIDIA CUDA or AMD HIP architectures. Attempting to maintain separate branches of the UPP codebase for CPUs and GPUs is computationally prohibitive, scales poorly, and introduces unacceptable technical debt for atmospheric scientists who must maintain the physics algorithms.

### The Configuration and Extensibility Bottleneck

Furthermore, the legacy UPP relies on an archaic configuration system driven by flat text or static XML files, such as `postcntrl.xml` and `post_avblflds.xml`. These control files dictate which variables are to be computed and outputted by the post-processor. However, because the scientific algorithms are hardcoded in procedural blocks rather than decoupled modules, introducing a new diagnostic product requires modifying deep, interconnected Fortran routines. This monolithic design actively hinders the rapid integration of novel algorithms, such as new machine-learning-based Snow-to-Liquid Ratio (SLR) calculations or advanced aviation hazard metrics.

The transition to the UFS demands a modular, extensible, and hardware-agnostic architecture. The UFS aims to simplify NOAA's operational suite from over twenty standalone forecast systems into a unified framework. To fulfill its role within the UFS, the UPP must evolve into a flexible library where scientific diagnostics can be plugged in seamlessly, memory is managed transparently across different hardware spaces, and data structures interoperate natively between the overarching forecast model and the post-processing computations.

## The C++ Orchestration Framework: The Backbone

The transition of the UPP from a monolithic application to a modular, highly extensible architecture requires a sophisticated orchestration mechanism. The `bbakernoaa/helm-project` is explicitly positioned to serve as this structural backbone. By leveraging modern C++ design paradigms, this backbone acts as a high-level driver that manages the complete lifecycle of the post-processing execution, abstracting the complexities of memory allocation, horizontal grid decomposition, MPI synchronization, and diagnostic sequencing from the underlying scientific algorithms.

### Shifting to an Object-Oriented Driver Architecture

The integration of the C++ backbone fundamentally shifts the software engineering paradigm of the UPP. Instead of a single, top-down executable pathway, the backbone introduces an object-oriented driver capable of dynamically constructing the computational environment. During initialization, the backbone parses modern configuration inputs (transitioning away from rigid legacy XML files toward more dynamic, schema-validated YAML or JSON structures), establishing the necessary MPI communicators and 2D grid decompositions across the computing nodes.

This abstraction establishes a critical separation of concerns. The core scientific algorithms—such as the thermodynamic equations required to calculate CAPE or the complex radiative transfer equations within the CRTM for satellite look-alike products —can be encapsulated into distinct, manageable C++ classes or plugins. The backbone manages when and where these plugins execute, providing them with the necessary data arrays and ensuring that boundary exchanges between MPI ranks are handled securely. Furthermore, this architecture establishes a safe interoperability boundary, ensuring that legacy Fortran routines that have not yet been ported can still be called via standard C bindings, allowing for a phased, incremental modernization of the codebase without disrupting operational continuity.

### Directed Acyclic Graph (DAG) Dependency Resolution

A highly advanced feature enabled by the C++ backbone is the implementation of Directed Acyclic Graph (DAG) dependency resolution for diagnostic computations. Atmospheric variables exhibit complex, interconnected dependencies. For example, computing simulated radar reflectivity may require the prior computation of specific humidity, cloud liquid water, cloud ice, and ambient temperature profiles. In the legacy procedural model, developers had to manually ensure that these prerequisite variables were computed in the correct sequential order within the code.

The modernized backbone will map all available diagnostic products and their prerequisites into a formal DAG. When a user or operational workflow requests a specific subset of output variables, the backbone traverses the graph to dynamically construct an optimal execution pathway. It ensures that all prerequisite fields are computed exactly once, caches them in optimized, short-lived memory allocations, feeds them into the downstream diagnostic algorithms, and then deallocates them immediately to minimize the overall memory footprint of the application. This intelligent orchestration prevents redundant calculations and optimizes cache utilization, leading to significant performance gains across the entire post-processing suite.

## Operational Middleware: The Helm Core Pillars (Halo, Axis, CONF, and LOGS)

To transform the UPP into a highly scalable, parallel system, the C++ orchestration framework must rely on dedicated middleware. The `bbakernoaa/helm-project` library provides four crucial pillars that manage distributed-memory boundaries, spatial and vertical coordinate structures, runtime configurations, and asynchronous logging.

### 1. The Axis Module (`helm::axis`): Advanced Remapping, Interpolation, and Grid Support

Vertical and horizontal spatial coordinate transformations are at the heart of post-processing raw model grid outputs. The `helm::axis` module is engineered to handle vertical coordinate transformations, horizontal grid staggerings, named NCEP GRIB grids, and grid-to-grid remapping without relying on legacy external libraries or expensive out-of-core calculations.

By natively handling named NCEP GRIB grids, `helm::axis` allows the post-processor to execute standardized coordinate, metadata, and grid projection lookups on the fly. This eliminates manual geometry reconstruction and hardcoded projection tables from the diagnostic pathway.

The vertical interpolation capabilities within `helm::axis` support a wide array of mathematical schemas:

*   **Tension Splines:** Essential for vertical interpolation of thermodynamic profiles (e.g., geopotential height $Z$ or temperature $T$) from native hybrid $\sigma$-pressure coordinates to constant pressure surfaces. By applying a tension factor to a cubic spline, this algorithm suppresses spurious, non-physical oscillations (overshoots and undershoots) in layers with sharp physical gradients, such as the tropopause or strong boundary-layer inversions. The 1D tension spline equation solved along each vertical column is represented as:

    $$
    \frac{d^2\psi}{dz^2}-\sigma^2\psi=f(z)
    $$

    where $\psi$ represents the interpolated variable, $z$ is the vertical coordinate, and $\sigma$ is the user-defined tension parameter that dampens oscillations.
*   **Nearest-Neighbor & Categorical:** Used for discrete land surface characteristics, soil types, vegetation indices, or cloud masks, where continuous mathematical interpolation would yield meaningless fractional values.
*   **Bilinear & Bicubic:** Standard continuous spatial interpolation techniques optimized for smooth variables like surface temperature or mean sea level pressure.
*   **Conservative & Second-Order Conservative:** Remapping methods that guarantee the conservation of integral physical quantities (such as total mass, energy, or tracer species concentrations) across different grids. This is vital for air quality modeling or chemical transport models.
*   **Vector Regridding:** Specialized algorithms to interpolate vector fields (like wind velocity $u$ and $v$ components) between staggered configurations (e.g., from the Arakawa C-grid used by the FV3 dycore to standard Lat-Lon grids). It automatically computes the necessary local wind rotation angles based on grid curvature to maintain physical directionality:

    $$
    u_{rot}=u\cos\theta-v\sin\theta
    $$
    $$
    v_{rot}=u\sin\theta+v\cos\theta
    $$

    where $\theta$ is the local rotation angle of the map projection relative to the destination grid coordinates.

### 2. The Halo Module (`helm::halo`): Asynchronous Boundary Exchanges

In massively parallel runs, the UFS horizontal domain is decomposed into multiple MPI ranks. Computing localized spatial derivatives—such as convective helicity, wind shear, or localized smoothing filters—requires diagnostic kernels to access "halo" or "ghost" cell regions from neighboring MPI domains.

*   The `helm::halo` module manages these multi-dimensional boundary exchanges natively over Kokkos-allocated memory spaces.
*   To eliminate the overhead of copying data between the host CPU and accelerator devices, `helm::halo` integrates with CUDA-Aware MPI, enabling direct GPU-to-GPU memory copies across InfiniBand interconnects.
*   By leveraging non-blocking MPI communicators wrapped in Kokkos asynchronous execution policies, the halo exchanges can be overlapped directly with core computational kernels, hiding network latencies.

### 3. The CONF Module (`helm::CONF`): Schema-Validated Configurations

*   The legacy XML-based config engines are replaced by `helm::CONF`, a lightweight, schema-validated configuration parser supporting YAML, JSON, or TOML formats.
*   During initialization, CONF parses the user input, instantiates the required diagnostics, and maps the relationships into the Directed Acyclic Graph (DAG) execution engine.
*   CONF dynamically calculates the exact memory allocations required for the transient arrays in the DAG, optimizing heap usage.

### 4. The Log Module (`helm::LOGS`): Thread-Safe Asynchronous Logging

*   Logging is a common bottleneck in high-rank MPI and multi-threaded environments. Synchronous print statements stall CPU and GPU execution threads.
*   `helm::LOGS` provides a lock-free, asynchronous logging queue that delegates diagnostic reporting to background CPU worker threads.
*   It features Rank-Filtering (ensuring only rank-0 prints verbose stdout while gathering multi-rank errors) and integrates with the Kokkos profiling API to trace execution wall-time per diagnostic algorithm.

| Module | Legacy Counterpart | Primary Modernized Responsibility |
|---|---|---|
| `helm::CONF` | `postcntrl.xml`, itag files | Parses configuration schemas, builds the computational DAG. |
| `helm::LOGS` | Sequential Fortran `write` / `print` | Asynchronous, thread-safe, rank-filtered diagnostic logging. |
| `helm::axis` | Hardcoded column index variables | Vertical coordinate transformations, horizontal remappings, vector regridding, and NCEP grid lookups. |
| `helm::halo` | Custom Fortran MPI wrappers | Manages multi-dimensional halo boundary exchanges over Kokkos memory spaces. |

## Hardware Abstraction and Performance Portability via Kokkos

To achieve true performance portability across all major high-performance computing (HPC) platforms, the modernized UPP utilizes the Kokkos programming model. Kokkos abstracts both the "execution space" (where the computational instructions run) and the "memory space" (where the data arrays reside), allowing atmospheric scientists and developers to write their computational kernels exactly once.

### Abstracting Execution Spaces

In the context of the UPP, diagnostic computations are typically highly parallelizable across the horizontal grid. For instance, the vertical interpolation of temperature fields from the FV3 dynamical core's native sigma-pressure hybrid coordinates to standard isobaric levels can be executed independently for every vertical atmospheric column. Kokkos facilitates this by replacing standard, architecture-specific nested `for` loops with generic `Kokkos::parallel_for` constructs, utilizing multidimensional execution policies such as `Kokkos::MDRangePolicy`.

When the modernized UPP is configured and compiled for a traditional CPU architecture (e.g., using CMake flags targeting Intel Xeon processors), Kokkos automatically maps these policies to highly optimized OpenMP threads, ensuring proper vectorization. When compiled for a GPU architecture, Kokkos maps the exact same C++ code to CUDA or HIP threads, organizing the computational blocks to ensure that memory access patterns remain coalesced and optimized for the GPU's specific hierarchical memory architecture.

### Memory Spaces and Data Placement

Beyond instruction execution, the physical placement of data is the primary bottleneck in heterogeneous computing. Transferring gigabytes of atmospheric state data across the PCIe bus between the host CPU memory and the device GPU memory is a high-latency operation that can entirely negate the computational speedup provided by the accelerator.

Kokkos addresses this through its memory space abstractions (`Kokkos::View`). The modernized UPP will utilize Kokkos to allocate multidimensional arrays in the optimal memory space for the targeted execution. When operating in inline mode, where the data already resides in memory, Kokkos can wrap existing data pointers using unmanaged views. If a diagnostic algorithm requires temporary scratch space to hold intermediate calculations (such as boundary layer heights or convective inhibition metrics), Kokkos dynamically allocates this memory in the highest-bandwidth space available (e.g., High Bandwidth Memory, or HBM, on a GPU), executes the kernel, and tears down the allocation. This rigorous control over data locality ensures that the GPU streaming multiprocessors are continuously fed with data, maximizing the floating-point operations per second (FLOPS) achieved by the UPP.

## Memory Interoperability and Multidimensional Views with C++23 mdspan

While Kokkos provides the critical abstraction for execution and basic memory views, managing complex multidimensional atmospheric arrays in C++ has historically been a cumbersome exercise, often leading to cache-unfriendly vector-of-vectors implementations or complex, error-prone pointer arithmetic. To solve this fundamental data structural issue, and to guarantee strict zero-copy interoperability with the Fortran-based UFS weather model, the UPP modernization plan integrates `std::mdspan`, a revolutionary feature introduced in the C++23 standard (utilized via the Kokkos `mdspan` backport to ensure maximum compiler compatibility across all HPC environments).

### The Architecture of mdspan

A `std::mdspan` is a non-owning multidimensional view of a contiguous sequence of objects in memory. It functions as a highly sophisticated semantic layer that understands the geometric shape of the data without taking ownership of the memory allocation itself. The architecture of `mdspan` is transformative for scientific computing because it explicitly separates four critical concerns that are often conflated in traditional array implementations:

1.  **Data Storage:** The underlying contiguous memory allocation, represented as a raw pointer or span.
2.  **Extents:** The dimensional shape of the multidimensional space (e.g., the number of grid points in the X, Y, and Z axes representing longitude, latitude, and vertical atmospheric levels).
3.  **Layout:** The specific mapping policy that translates multidimensional indices into a flat, 1D memory address.
4.  **Access:** The policy dictating how individual elements are retrieved (e.g., standard access, atomic access, or restricted access).

By leveraging `mdspan`, the modernized C++ UPP algorithms can construct highly readable, multidimensional subscripts (e.g., `temperature_field[i, j, k]`) without incurring the overhead of deep copies or opaque pointer math. This ensures that the code describing the atmospheric physics remains mathematically expressive and cleanly decoupled from the underlying hardware-specific memory management.

### Bridging the C++ and Fortran Divide: The Layout Crisis

Perhaps the most critical technical challenge in modernizing the UPP is maintaining zero-copy memory interoperability with the legacy Fortran codebase of the UFS weather model. The UFS FV3 dynamical core, written in Fortran, allocates and manages massive 3D arrays representing the state of the atmosphere. Fortran and C++ possess inherently different native memory layouts. C++ utilizes a row-major layout, where consecutive elements of a row are placed contiguously in physical memory. Fortran utilizes a column-major layout, where consecutive elements of a column are contiguous.

If a C++ component attempts to read a Fortran-allocated array using standard C++ row-major nested loops, the processor will fetch data out of order, resulting in massive cache misses. In high-performance computing, memory bandwidth—not CPU cycle time—is the primary constraint. Severe cache thrashing can degrade algorithmic performance by orders of magnitude. Historically, bridging this divide required explicitly transposing or copying massive 3D atmospheric arrays when passing data from Fortran routines to C++ routines, wasting critical memory bandwidth and RAM.

### Resolving the Crisis with layout_left

The `mdspan` library resolves this language interoperability hurdle natively through its layout policies, effectively eliminating the need for data transposition. The C++23 standard provides several built-in layout policies, which can be applied directly to the multidimensional view.

| Layout Policy | Linguistic Equivalent | Memory Continuity | Mathematical Offset Mapping (3D Array) |
|---|---|---|---|
| `std::layout_right` | C, C++, Python (NumPy default) | Row-major. The rightmost index provides stride-1 (contiguous) access to underlying memory. | $offset=k+extents \times (j+extents \times i)$ |
| `std::layout_left` | Fortran, Matlab, R | Column-major. The leftmost index provides stride-1 access. | $offset=i+extents \times (j+extents \times k)$ |
| `std::layout_stride`| N/A (Custom) | Unstructured striding allowing arbitrary offsets for each dimension, useful for sub-grid slicing or halo regions. | $offset=i \times s_0+j \times s_1+k \times s_2$ |

By defining the multidimensional views of the incoming FV3 dynamic core data with `std::layout_left`, the C++ backbone can directly wrap the raw memory pointers passed from the Fortran-based UFS. The C++ code can then iterate over the data using standard syntax, while the `mdspan` layout policy seamlessly translates the indices into the correct Fortran-native memory addresses behind the scenes. The data remains strictly in place, and the C++ diagnostic algorithms execute over it flawlessly, achieving true zero-copy interoperability and preserving strict hardware cache coherency.

## Overcoming I/O Bottlenecks with Asynchronous I/O (AMIO)

The generation, compression, and dissemination of meteorological data constitute one of the most I/O-intensive operations in all of computational science. Traditional implementations of the UFS and UPP have relied heavily on synchronous writing mechanisms. In a synchronous execution model, the entire forecast integration (the mathematical stepping forward of the atmosphere in time) must halt while the raw history files or post-processed GRIB2 products are written to disk.

As numerical models have advanced, resolutions have increased dramatically. The FV3 dynamical core now routinely operates at sub-3km horizontal resolutions for regional models like the HRRR and RRFS, and vertical levels have expanded beyond 127 layers. The sheer volume of output data generated at these resolutions creates severe computational bottlenecks. If the processors are waiting on disk I/O, they are not computing atmospheric physics, threatening the strict, time-sensitive operational forecasting windows mandated by the National Weather Service (NWS).

### The Asynchronous Architecture

To circumvent these limitations, the UPP modernization plan integrates a sophisticated Asynchronous I/O (AMIO) architecture. AMIO fundamentally separates the forecast integration mathematics from the physical writing of output files.

Within the AMIO framework, the primary computational nodes—dedicated exclusively to the FV3 model integration and the Kokkos-accelerated UPP diagnostic routines—do not interact directly with the POSIX file system. Instead, a dedicated pool of write tasks is established, mapped either to separate MPI ranks or distinct background threads. When the inline UPP finishes processing the diagnostic fields for a specific forecast hour (e.g., FH=03), these fields are rapidly passed through memory to the AMIO write component.

Crucially, the primary computation nodes immediately resume integrating the next forecast hour (e.g., moving toward FH=04). The AMIO component handles the computationally heavy lifting of data compression, packing, metadata tagging, and disk or network writing entirely in the background. This decoupling requires sophisticated memory buffering and fencing to ensure that the forecast model does not overwrite the memory addresses of the diagnostic fields before the AMIO component has finalized the output. The C++ backbone manages this via asynchronous message passing, utilizing Kokkos' deep copy utilities to efficiently duplicate the required data into the protected AMIO staging area.

### Multi-Format Output: GRIB2, NetCDF, and Zarr

The modern atmospheric science community is highly diverse, ranging from operational forecasters who rely on highly compressed legacy formats for rapid transmission, to academic researchers who require hierarchical metadata, to cloud-engineers who require object-storage optimized data lakes. Consequently, the AMIO implementation cannot be restricted to a single file type; it must seamlessly support three distinct formats, each serving a strategic purpose within the broader NOAA data pipeline.

#### GRIB2: Native Integration of NCEPLIBS-g2c

The Gridded Binary Second Edition (GRIB2) format is the World Meteorological Organization (WMO) international standard for exchanging gridded meteorological data. GRIB2 is inherently self-describing, though it relies heavily on external lookup tables to map numerical codes to specific atmospheric variables and vertical levels.

Its primary advantage, and the reason it remains the bedrock of operational meteorology, is its extremely aggressive compression capabilities. By utilizing complex packing algorithms (e.g., JPEG2000 or CCSDS compression) and undefined value masks, GRIB2 can achieve massive size reductions. For example, specific fields like sea surface temperature masks can be compressed by 92%, effectively storing grid point values using less than one bit per point by ignoring undefined values over landmasses.

Rather than deploying heavyweight external command-line utilities like `wgrib2` or third-party packages like ecCodes, the AMIO architecture natively integrates the NCEPLIBS-g2c library. This GRIB2 C library is linked directly into the C++ driver's asynchronous threads to encode, decode, and pack grids. This native integration reduces library dependency complexity on NOAA systems while delivering extremely fast, thread-safe asynchronous data encoding directly out of memory.

#### NetCDF: The Research Standard

Network Common Data Form (NetCDF) is specifically designed to facilitate the creation, access, and sharing of array-oriented scientific data. Fully compliant with CF (Climate and Forecast) and COARDS metadata conventions, NetCDF includes highly descriptive headers that detail the layout of the data arrays and arbitrary file metadata as name/value attributes, completely eliminating the need for external lookup tables.

NetCDF is heavily utilized by academic researchers and is widely supported by standard visualization and analysis tools, such as Python's xarray, the NCAR Command Language (NCL), and Climate Data Operators (CDO). The AMIO implementation will leverage parallel NetCDF (PnetCDF) capabilities, allowing multiple MPI ranks to write discrete chunks of data to a single NetCDF history file simultaneously, significantly reducing the I/O overhead of aggregating thousands of small files post-run.

#### Zarr: The Cloud-Optimized Future

As NOAA aggressively migrates massive datasets to commercial cloud infrastructure (e.g., Amazon Web Services, Google Cloud Platform) under initiatives like the Open Data Registry, traditional file formats exhibit severe limitations. GRIB2 and NetCDF files are fundamentally designed for POSIX file systems and are generally read sequentially. Accessing a specific bounding box of data (e.g., extracting a time-series of surface temperature over a single city) from a cloud object store like Amazon S3 requires downloading the entire multi-gigabyte GRIB2 file, or performing complex byte-range lookups that suffer from high latency overheads.

Zarr solves this paradigm mismatch by entirely reorganizing multidimensional arrays into highly compressed, independent sub-domain chunks. Rather than storing a forecast output as a single massive file, Zarr stores the metadata in a lightweight JSON file and stores each discrete chunk (e.g., a $150 \times 150$ grid point subdomain for a specific variable and lead time) as a separate object.

This chunked architecture allows open-source cloud libraries to execute thousands of parallel HTTP GET requests to retrieve only the specific spatial and temporal data required for an analysis, entirely bypassing data that is not needed. The integration of Zarr into the AMIO pipeline represents a transformative leap in data accessibility. Historical case studies demonstrate that reformatting High-Resolution Rapid Refresh (HRRR) model output into Zarr stores reduces data retrieval times for specific time-series analyses by a factor of 40 compared to traditional GRIB2 archives hosted on AWS. By allowing the modernized UPP to output Zarr natively via the AMIO asynchronous threads, NOAA bypasses computationally expensive post-processing conversion steps, delivering real-time, cloud-ready data instantly to global users.

| Format | Primary Use Case | Compression Strategy | Storage Paradigm | Cloud Object Storage (S3) Performance |
|---|---|---|---|---|
| GRIB2 | Operational NWP dissemination, WMO standards, automated alerting | Highly aggressive, specialized meteorological packing (e.g., JPEG2000, bit-mapping) | Sequential binary messages with external table dependencies; integrated via NCEPLIBS-g2c | Poor (requires complex indexing for byte-range reads, high latency) |
| NetCDF4 (HDF5) | Academic research, complex hierarchical data modeling | Standard DEFLATE / zlib | Single monolithic file (traditionally) with internal hierarchical structure | Moderate (requires specialized tooling like Kerchunk for optimal performance) |
| Zarr | Cloud-native data lakes, highly parallel cloud analytics, machine learning training sets | Blosc, LZ4, Zstandard (highly customizable per array) | Chunked object storage (thousands of discrete keys mapping to small, independent arrays) | Exceptional (native parallel REST API reads enable 40x speedups) |

## The Python Binding Layer: Zero-Copy Integration for Data Science and Machine Learning

The modern weather enterprise is increasingly defined by hybrid workflows that combine traditional numerical weather prediction with advanced data science and artificial intelligence. While operational forecasting requires the extreme performance of compiled C++ and Fortran inline cores, research scientists, model evaluators, and machine learning engineers prototype almost exclusively in Python. Bridging this linguistic divide is critical for accelerating Research-to-Operations (R2O) pipelines. The modernized UPP addresses this by exposing a high-performance Python interface directly to the underlying `helm` computational kernels.

### 1. Transitioning to nanobind for Lightweight Bindings

Historically, exposing C++ libraries to Python was accomplished using `pybind11`. However, in highly templated C++ architectures—especially those leveraging Kokkos and multiple dimension configurations—`pybind11` incurs significant compilation overhead, bloated binary sizes, and runtime dispatch delays.

To resolve these limitations, the UPP Python interface will rely on `nanobind`. Developed as the modern, lightweight successor to `pybind11`, `nanobind` yields binaries that are approximately 2.5 times smaller and provides runtime call overheads that are roughly 1.3 times faster. This reduction in overhead is particularly critical for high-frequency call loops, such as calling post-processing routines at every fine time step of a regional ensemble forecast.

### 2. Bypassing the Memory Wall via the Buffer Protocol

The primary bottleneck when passing massive multi-dimensional atmospheric datasets between C++ and Python is memory copying. Serializing a 50GB atmospheric state array into Python-native list objects or standard class wrappers inevitably leads to a "Memory Wall," causing out-of-memory (OOM) allocation crashes.

The modernized UPP's Python wrapper bypasses this entire serialization layer by leveraging the Python Buffer Protocol. Utilizing `nanobind`'s deep integration with NumPy, the unmanaged device or host memory pointers managed by Kokkos views and C++23 `mdspan` objects are exposed directly to Python as read-only `numpy.ndarray` views with zero copies.

```python
import numpy as np
import pyupp as upp

# Wrap a raw, pre-allocated NumPy array representing model temperature
# directly into a zero-copy unmanaged mdspan view with layout_left
temp_data = np.load("temp_3d_state.npy")
upp_view = upp.wrap_array(temp_data, layout="left") # Zero allocation

# Execute vertical tension spline interpolation via compiled C++/Kokkos core
isobaric_temp = upp.axis.interpolate_vertical(upp_view, targets=)
```

On the Python side, the resulting array is treated as a standard NumPy or PyTorch-compatible array, yet accessing its elements triggers zero Python heap allocations. When executing on accelerators, the C++ layer can handover CUDA device pointers directly to Python libraries like CuPy or PyTorch using the CUDA Array Interface, maintaining end-to-end GPU data residency.

### 3. Synergy with ML-Based Physics and Python Toolkits

Exposing the post-processor's calculations directly to Python opens up transformative use cases for machine learning and evaluation:

*   **Inline Corrective Machine Learning:** Data scientists can easily insert deep learning inference models (e.g., PyTorch or JAX) directly into the post-processing pipeline. Incoming model state arrays can be processed by a convolutional neural network (CNN) or a Random Forest model in Python to correct systematic biases (such as the GFS convective rain biases) before saving the final diagnostics.
*   **Seamless Model Evaluation:** Exposing these diagnostics directly to Python ensures instant interoperability with NOAA's Model and Observation Evaluation Toolkit (MONET) and custom evaluation libraries. It also allows developers to utilize high-performance Python-based regridding packages like `xregrid`, which leverages ESMPy for optimized sparse-matrix interpolation, bridging the gap between raw model outputs and standardized research grids.

## Dual Execution Modalities: Inline and Offline Modes

A core mandate of the UPP modernization is extreme flexibility in how and where post-processing occurs. Historically, post-processing was strictly an offline, serial task: the forecast model would run to completion (or reach an output interval), write massive raw history files (often in proprietary formats like NEMSIO) to the parallel file system, and subsequently, the UPP executable would be launched. The UPP would read these massive files back into memory, compute the requested diagnostics, and write the final GRIB2 products. This approach incurs massive, redundant I/O overhead. To address this, the modernized architecture is explicitly engineered to support two distinct execution modalities: Inline UFS execution and an Offline C++ driver.

### Inline UFS Execution: The Operational Standard

Inline execution represents the most computationally efficient paradigm and is targeted as the primary operational mode for global models like the GFS and regional ensembles like the RRFS. In this mode, the UPP is not compiled as a standalone executable; rather, it is compiled as a shared library (`upp::upp`) that is statically linked directly into the `ufs-weather-model`.

The workflow is highly streamlined. When the UFS model reaches a designated forecast output hour, the FV3 dynamical core and the Common Community Physics Package (CCPP) complete their mathematical integration steps. Instead of writing the raw prognostic variables (e.g., 3D fields of temperature, zonal and meridional winds, specific humidity) to disk, the UFS executes a direct sub-routine call to the UPP inline library interface.

Using the Kokkos and `mdspan` abstractions detailed previously, the UPP library is passed the raw Fortran memory pointers directly from the UFS model state. The C++ backbone immediately initiates the diagnostic computations—calculating fields like visibility, gust wind, ceiling, and derived isobaric levels—entirely in memory. Because absolutely no disk I/O occurs during this data handover, the latency is effectively zero. Once the diagnostics are computed and stored in the Kokkos execution space, they are handed to the AMIO component for asynchronous writing. This inline capability radically speeds up the entire forecast system by completely eliminating the intermediate read and write steps of raw model history files.

### Offline C++ Driver: The Research Standard

Despite the immense operational efficiency of inline execution, a standalone, offline capability remains critical for the broader scientific community, retrospective climate reanalysis, and specialized verification workflows like METplus. The modernized UPP will feature a dedicated Offline C++ Driver built atop the `bbakernoaa/helm-project` architecture.

In offline mode, the UPP functions as a traditional standalone executable. The C++ driver initializes an independent MPI environment and utilizes the AMIO read interface to ingest raw model history files from disk or cloud storage. It reconstructs the multidimensional `mdspan` views from the stored data, processes the diagnostic DAG as requested by the user's configuration, and writes the finalized output products.

This offline driver is highly advantageous for hierarchical system development (HSD). For instance, a researcher developing a new machine-learning-based precipitation type algorithm can run the offline C++ driver over years of archived FV3 history files without needing the immense computational resources, or the complex initialization data, required to run the full `ufs-weather-model`. Furthermore, the offline driver is ideal for cloud-native deployment; it can be packaged into lightweight containers and deployed across distributed clusters, scaling dynamically to post-process petabytes of stored climate data asynchronously.

## Deployment across HPC and Cloud Environments

The unified nature of the modernized UPP dictates that the codebase must compile and execute flawlessly across the highly diverse infrastructure utilized by NOAA, academic universities, and private sector partners. This deployment strategy explicitly targets two primary environments: traditional Research and Development High-Performance Computing Systems (RDHPCS) and scalable commercial cloud providers.

### NOAA RDHPCS Deployments and Build Systems

NOAA operates several massive supercomputing clusters dedicated to operational forecasting and research, including systems like Hera, Hercules, and NCAR's Derecho. These systems utilize traditional bare-metal infrastructures, featuring massive Lustre or GPFS parallel file systems, high-speed InfiniBand network interconnects, and strict module-based environment controls.

The modernized UPP will be deployed on these systems utilizing the `spack-stack` project. `spack-stack` is a comprehensive software management tool designed to standardize the installation of the NCEPLIBS prerequisites—such as `w3emc`, `bacio`, `nemsio`, `sp`, `g2c`, and the `netcdf-c`/`netcdf-fortran` libraries—across all supported NOAA platforms.

The compilation of the C++ driver and the underlying Kokkos kernels will be heavily integrated with CMake, entirely replacing legacy, platform-specific Makefiles. This provides robust, dynamic hardware detection. During the CMake configuration phase on an HPC system (e.g., executing `cmake .. -DINLINE_POST=ON -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}`), the build scripts will intelligently detect the presence of specific hardware accelerators. If NVIDIA A100 GPUs are detected on the compute nodes, CMake will automatically compile the Kokkos kernels with the CUDA backend; if only Intel Xeon CPUs are present, it will default to optimized OpenMP threading. This dynamic compilation ensures maximum performance portability without requiring manual intervention or deep architectural knowledge from the research scientist.

### Cloud-Native and Containerized Deployments

To fulfill the Earth Prediction Innovation Center's (EPIC) mandate of democratizing access to the UFS, the UPP must operate smoothly outside of restricted government hardware enclaves. The modernized architecture embraces cloud-native deployments through comprehensive containerization strategies.

Automated Continuous Integration/Continuous Deployment (CI/CD) pipelines will continuously build and publish Docker images containing the full UPP software stack, including the offline C++ driver, Kokkos, `mdspan` libraries, and all AMIO dependencies. For HPC systems or academic clusters that restrict root access (making Docker unusable), these images are natively convertible to Singularity or Apptainer containers.

In a commercial cloud environment (such as AWS EC2 or Google Compute Engine), the offline driver can completely bypass traditional parallel file systems. By utilizing the AMIO Zarr integration, the containerized UPP can stream raw history data directly from an AWS S3 bucket, compute severe weather diagnostics (e.g., helicity, simulated radar reflectivity), and stream the resulting Zarr chunks directly back to another S3 bucket in real-time. This architecture enables highly elastic, serverless meteorological pipelines. An organization can spin up thousands of Kubernetes pods simultaneously to post-process a massive 30-member global ensemble forecast without encountering the locking bottlenecks or bandwidth limits inherent in traditional POSIX file systems.

## Risk Mitigation, Validation, and Strategic Roadmap

A modernization effort of this magnitude—touching the core data output mechanisms of national operational weather models—inherently carries significant technical and operational risks. These risks must be actively managed through a rigorous validation framework and a meticulously phased implementation roadmap.

### Managing Numerical Divergence and Memory Bloat

The primary risk in translating atmospheric physics algorithms from procedural Fortran to modern C++ is numerical divergence. Because the UPP outputs are heavily relied upon to train downstream machine learning algorithms, drive severe weather alerts, and initialize aviation routing software, any drift in the diagnostic output—even a fraction of a degree change in a simulated brightness temperature or a slight rounding error in a CAPE calculation—can trigger systemic forecasting anomalies.

To mitigate this, the project will rely heavily on the UFS hierarchical testing framework. Every pull request modifying a scientific kernel will undergo automated bit-for-bit (B4B) or strictly bounded tolerance verification, directly comparing the output of the new Kokkos C++ kernels against the output of the legacy `ncep_post.fd` Fortran algorithms to ensure absolute mathematical fidelity.

A secondary risk involves the memory footprint of the Asynchronous I/O (AMIO) buffers. While AMIO prevents the forecast model from blocking during disk writes, the data buffered in RAM prior to being written to disk can be substantial, especially for high-resolution grids. If the UFS model operates on compute nodes with limited memory capacity, unchecked AMIO buffers could lead to out-of-memory (OOM) fatal crashes. Mitigation requires implementing dynamic back-pressure mechanisms within the C++ backbone. If the AMIO queue approaches a critical RAM utilization threshold, the C++ driver will automatically signal the UFS integration to pause briefly, allowing the I/O threads to clear the backlog. This ensures the system gracefully degrades from fully asynchronous to partially synchronous behavior to maintain overall stability.

### Phased Implementation Roadmap

To ensure uninterrupted operational service, the modernization plan will be executed in a meticulously sequenced, five-phase rollout.

| Implementation Phase | Strategic Focus Area | Key Deliverables and Architectural Outcomes |
|---|---|---|
| **Phase 1: Foundation** | C++ Backbone Integration | - Instantiate the `bbakernoaa/helm-project` C++ framework.<br>- Replace legacy XML configuration parsing with dynamic schemas.<br>- Integrate Directed Acyclic Graph Resolver (DAGR) for automated diagnostic dependency resolution.<br>- Establish the CMake build system linked with `spack-stack` and NCEPLIBS. |
| **Phase 2: Abstraction** | Memory Portability via Kokkos and `mdspan` | - Map FV3 atmospheric fields to the Kokkos `mdspan` backport using `std::layout_left` for Fortran continuity.<br>- Integrate `helm::axis` for advanced vertical interpolations, vector regridding, and NCEP grid transformations.<br>- Port initial algorithmic kernels (e.g., vertical interpolations) to Kokkos `parallel_for` execution spaces. |
| **Phase 3: Data Velocity** | AMIO Integration and Multi-Format Support | - Implement asynchronous thread pools for data dissemination.<br>- Leverage native NCEPLIBS-g2c within AMIO for high-performance GRIB2 encoding and pack the output.<br>- Build and test Zarr chunking for direct-to-S3 object storage. |
| **Phase 4: Operationality** | Dual Execution Capability Validation | - Embed the UPP shared library (`upp::upp`) directly into the `ufs-weather-model` for zero-copy inline testing.<br>- Finalize the standalone Offline C++ Driver for retrospective data processing. |
| **Phase 5: Democratization** | Cloud Deployment and Benchmarking | - Deploy containerized workflows (Docker/Singularity) to EPIC community repositories.<br>- Conduct massive parallel benchmarking on AWS evaluating Zarr vs GRIB2 retrieval times. |

## Strategic Synthesis and Future Outlook

The modernization of the NOAA Unified Post Processor represents far more than a routine software translation effort; it is a foundational paradigm shift required to secure the computational future of the Unified Forecast System. By adopting the `bbakernoaa/helm-project` as the orchestrating C++ driver, the architecture pivots from a rigid, monolithic structure to a highly flexible, modular framework capable of adapting to rapid advancements in atmospheric science and artificial intelligence.

The integration of the Kokkos programming model guarantees that NOAA and the broader meteorological community are insulated against the shifting landscape of hardware vendors. This abstraction enables the exact same post-processing codebase to execute with maximum efficiency across AMD CPUs, NVIDIA GPUs, and whatever novel architectures eventually dominate the exascale computing era. The strategic implementation of the C++23 `mdspan` library, specifically utilizing `layout_left` definitions, elegantly resolves the decades-old friction between C++ and Fortran memory allocations. This ensures zero-copy memory access that empowers lightning-fast inline execution within the `ufs-weather-model`, eliminating the historical lag of disk-based handovers.

Simultaneously, the Asynchronous I/O (AMIO) architecture breaks the most significant bottleneck in modern high-resolution NWP. By natively supporting GRIB2 to satisfy rigid operational mandates via NCEPLIBS-g2c, NetCDF for deep academic research, and Zarr for cloud-native analytics, the modernized UPP ensures that NOAA's data is highly accessible, rapidly generated, and optimally structured for downstream consumption. Whether deployed tightly coupled inline on the world's most powerful bare-metal supercomputers or spun up dynamically as an offline container in highly scalable AWS Kubernetes clusters, the modernized Unified Post Processor will provide the high-fidelity, high-velocity atmospheric intelligence necessary to sustain a truly Weather-Ready Nation.

## Works cited

1.  Unified Post Processor (UPP) - Earth Prediction Innovation Center - NOAA, https://www.epic.noaa.gov/unified-post-processor/
2.  Unified Post Processor (UPP) - Earth Prediction Innovation Center, https://epic-dev.noaa.gov/unified-post-processor/
3.  NOAA-EMC/UPP - GitHub, https://github.com/NOAA-EMC/UPP
4.  Unified Post Processor Users Guide, https://upp.readthedocs.io/_/downloads/en/latest/pdf/
5.  Unified Forecast System - NOAA, https://ufs.epic.noaa.gov/
6.  UFS R2O - Virtual Lab - NOAA VLab, https://vlab.noaa.gov/web/ufs-r2o
7.  UPP/sorc/ncep_post.fd/SET_LVLSXML.f at develop · NOAA-EMC/UPP · GitHub, https://github.com/NOAA-EMC/UPP/blob/develop/sorc/ncep_post.fd/SET_LVLSXML.f
8.  UPP/sorc/ncep_post.fd/CALWXT_BOURG.f at develop · NOAA-EMC/UPP · GitHub, https://github.com/NOAA-EMC/UPP/blob/develop/sorc/ncep_post.fd/CALWXT_BOURG.f
9.  Using Cloud Computing to Analyze Model Output Archived in Zarr Format - the NOAA Institutional Repository, https://repository.library.noaa.gov/view/noaa/60380
10. NOAA-EMC/ufsatm - GitHub, https://github.com/NOAA-EMC/ufsatm
11. Development of the Unified Post Processor (UPP) to Generate GEFS and SFS Products - NOAA VLab, https://vlab.noaa.gov/documents/17693964/39361920/Day2_PosterSession2_Meng.pdf
12. How to Setup UPP Grid_Mapping Parameter · NOAA-EMC UPP · Discussion #635 - GitHub, https://github.com/NOAA-EMC/UPP/discussions/635
13. SPARK Tool - NOAA Weather Program Office, https://wpo.noaa.gov/spark/
14. UFS-AQM Community Version Capability Announcement, https://www.epic.noaa.gov/ufs-aqm-capability-announcement/
15. ufs-community/CATChem - GitHub, https://github.com/ufs-community/CATChem
16. The Role and Future of mdspan for Performance-Portable Multidimensional Arrays in C++ | by Dikhyant Krishna Dalai | Medium, https://medium.com/@dikhyantkrishnadalai/the-role-and-future-of-mdspan-for-performance-portable-multidimensional-arrays-in-c-1463c0335d66
17. What is an mdspan, and what is it used for? - Stack Overflow, https://stackoverflow.com
