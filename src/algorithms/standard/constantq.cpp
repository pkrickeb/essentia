  /*
   * Copyright (C) 2006-2013  Music Technology Group - Universitat Pompeu Fabra
   *
   * This file is part of Essentia
   *
   * Essentia is free software: you can redistribute it and/or modify it under
   * the terms of the GNU Affero General Public License as published by the Free
   * Software Foundation (FSF), either version 3 of the License, or (at your
   * option) any later version.
   *
   * This program is distributed in the hope that it will be useful, but WITHOUT
   * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   * details.
   *
   * You should have received a copy of the Affero GNU General Public License
   * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
   */

  #include "ConstantQ.h"
  #include "essentia.h"

  using namespace std;
  using namespace essentia;
  using namespace standard;

  const char* ConstantQ::name = "ConstantQ";
  const char* ConstantQ::description = DOC("\n"
                                      "\n"
                                      "\n"
                                      "References:\n"
                                      "  [1] Constant Q transform - Wikipedia, the free encyclopedia,\n"
                                      "  hhttps://en.wikipedia.org/wiki/Constant_Q_transform\n\n"
                                      );



  //---------------------------------------------------------------------------
  // nextpow2 returns the smallest integer n such that 2^n >= x.
  static double nextpow2(double x) {
      double y = ceil(log(x)/log(2.0));
      return(y);
  }

  static double squaredModule(const std::complex<double> xx) {
      std::complex<double> multComplex = xx * xx;
      return multComplex.real() + multComplex.imag();
  }

  //---------------------------------------------------------------------------


  ConstantQ::~ConstantQ() {

      delete _fft;

    
    // ForcedMutexLocker lock(globalConstantQMutex);

    // we might have called essentia::shutdown() before this algorithm goes out
    // of scope, so make sure we're not doing stupid things here
    // This will cause a memory leak then, but it is definitely a better choice
    // than a crash (right, right??? :-) )
    // if (essentia::isInitialized()) {
    //     vDSP_destroy_fftsetup(fftSetup);
    //     free(accelBuffer.realp);
    //     free(accelBuffer.imagp);
    // }
  }

  void ConstantQ::compute() {

    const std::vector<std::complex<double> >& signal = _signal.get();
    std::vector<std::complex<double> >& constantQ = _constantQ.get();

     if (!m_sparseKernel) {
         throw EssentiaException("ERROR: ConstantQ::compute: Sparse kernel has not been initialised");
      }

      SparseKernel *sk = m_sparseKernel;

      // for (unsigned row=0; row<m_uK; row++)
      // {
      //   CQRe[ row ] = 0;
      //   CQIm[ row ] = 0;
      // }
      constantQ.assign(_FFTLength, 0.0 + 0.0j);



      const unsigned *fftbin = &(sk->_sparseKernelIs[0]);
      const unsigned *cqbin  = &(sk->_sparseKernelJs[0]);
      const double   *real   = &(sk->_sparseKernelReal[0]);
      const double   *imag   = &(sk->_sparseKernelImag[0]);
      const unsigned int sparseCells = sk->_sparseKernelReal.size();

      for (unsigned i = 0; i<sparseCells; i++)
      {
        const unsigned row = cqbin[i];
        const unsigned col = fftbin[i];
        const double & r1  = real[i];
        const double & i1  = imag[i];
        const double & r2  = signal.at( _FFTLength - col - 1 ).real();
        const double & i2  = signal.at( _FFTLength - col - 1 ).imag();
    // add the multiplication

        constantQ.at(row) += complex <double>((r1*r2 - i1*i2), (r1*i2 + i1*r2));

        // CQRe[ row ] += (r1*r2 - i1*i2);
        // CQIm[ row ] += (r1*i2 + i1*r2);
      }

    
  }


  void ConstantQ::configure() {

    _sampleRate = parameter("sampleRate").toDouble();
    _minFrequency = parameter("minFrequency").toDouble();
    _maxFrequency = parameter("maxFrequency").toDouble();
    _binsPerOctave = parameter("binsPerOctave").toInt();
    _threshold = parameter("threshold").toDouble();

    _dQ = 1/(pow(2,(1/(double)_binsPerOctave))-1);  // Work out Q value for Filter bank
    _uK = (unsigned int) ceil(_binsPerOctave * log(_maxFrequency/_minFrequency)/log(2.0));  // No. of constant Q bins


    _FFTLength = (int) pow(2, nextpow2(ceil( _dQ * _sampleRate / _minFrequency )));

    _hop = _FFTLength/8; // <------ hop size is window length divided by 32


    // _sampleRate = Config.FS;
    // m_FMin = Config.min;    // min freq
    // m_FMax = Config.max;    // max freq
    // m_BPO = Config.BPO;   // bins per octave
    // m_CQThresh = Config.CQThresh;// ConstantQ threshold for kernel generation



  /* begin Sparse Kernel */

    SparseKernel *sk = new SparseKernel();


    // initialise temporal kernel with zeros, twice length to deal w. complex numbers
    //std::vector<double> hammingWindowRe(_FFTLength, 0.0);
    //std::vector<double> hammingWindowIm(_FFTLength, 0.0);
   
    std::vector<std::complex<double> >  hammingWindow(_FFTLength, 0.0 + 0.0j);

    //std::vector<double> transfHammingWindowRe(_FFTLength, 0.0);
    //std::vector<double> transfHammingWindowIm(_FFTLength, 0.0);

    std::vector<std::complex<Real> >  transfHammingWindowR(_FFTLength, 0.0 + 0.0j);


    sk->_sparseKernelIs.reserve( _FFTLength*2 );
    sk->_sparseKernelJs.reserve( _FFTLength*2 );
    sk->_sparseKernelReal.reserve( _FFTLength*2 );
    sk->_sparseKernelImag.reserve( _FFTLength*2 );
    //sk->_sparseKernel.imag().reserve( _FFTLength*2 );


    // for each bin value K, calculate temporal kernel, take its fft to
    //calculate the spectral kernel then threshold it to make it sparse and
    //add it to the sparse kernels matrix
    double squareThreshold = _threshold * _threshold;


    for (unsigned k = _uK; k--; )
      {
          //hammingWindowRe.assign(_FFTLength, 0.0);
          //hammingWindowIm.assign(_FFTLength, 0.0);

          hammingWindow.assign(_FFTLength, 0.0 + 0.0j);

      

    // Computing a hamming window
    const unsigned hammingLength = (int) ceil( _dQ * _sampleRate / ( _minFrequency * pow(2,((double)(k))/(double)_binsPerOctave)));

          unsigned origin = _FFTLength/2 - hammingLength/2;

    for (unsigned i=0; i<hammingLength; i++)
    {
        const double angle = 2 * M_PI * _dQ * i/hammingLength;
        const double real = cos(angle);
        const double imag = sin(angle);
        const double absol = hamming(hammingLength, i)/hammingLength;

        hammingWindow.at( origin + i) = complex <double>(absol*real, absol*imag);
        //hammingWindow[ origin + i ].imag() = absol*imag;
    }

    std::complex<double> temp;

          for (unsigned i = 0; i < _FFTLength/2; ++i) {
              //double temp = hammingWindowRe[i];
              temp = hammingWindow.at(i); //
              hammingWindow.at(i) = hammingWindow.at(i + _FFTLength/2);
              hammingWindow.at(i + _FFTLength/2) = temp;
              // hammingWindowRe[i] = hammingWindowRe[i + _FFTLength/2];
              // hammingWindowRe[i + _FFTLength/2] = temp;
              // temp = hammingWindowIm[i];
              // hammingWindowIm[i] = hammingWindowIm[i + _FFTLength/2];
              // hammingWindowIm[i + _FFTLength/2] = temp;
          }


      //m_FFT.process( 0, hammingWindowRe, hammingWindowIm, transfHammingWindowRe, transfHammingWindowIm );
      std::vector<std::complex<Real> >  hammingWindowR(hammingWindow.begin(), hammingWindow.end());

      

      _fft->input("frame").set(hammingWindowR);
      _fft->output("fft").set(transfHammingWindowR);
      _fft->compute();

       std::vector<std::complex<double> >  transfHammingWindow(transfHammingWindowR.begin(), transfHammingWindowR.end());

       //std::vector<std::complex<double> > tempHamming(transfHammingWindowR.begin(), transfHammingWindowR.end());
      std::vector<std::complex<double> >  :: iterator itHamm = transfHammingWindow.end()-1;
      
      transfHammingWindow.resize(_FFTLength);

      for (unsigned i = 0; i < _FFTLength/2; ++i)
      {
        transfHammingWindow.push_back(*itHamm--);
        /* code */
      }




      for (unsigned j=0; j<( _FFTLength ); j++)
       {
        // perform thresholding
        const double squaredBin = squaredModule( transfHammingWindow.at(j));
        //std::cout << transfHammingWindow.at(j);
        if (squaredBin <= squareThreshold) continue;

        // Insert non-zero position indexes, doubled because they are floats
        
        sk->_sparseKernelIs.push_back(j);
        sk->_sparseKernelJs.push_back(k);

        // // take conjugate, normalise and add to array sparkernel
        sk->_sparseKernelReal.push_back( transfHammingWindow.at( j ).real()/_FFTLength);
        sk->_sparseKernelImag.push_back( -transfHammingWindow.at( j ).imag()/_FFTLength);
      }

    }

    m_sparseKernel = sk;
    /* end of Sparse Kernel */



  }

